/***************************************************************************
 *   Copyright (C) 2013 by Torsten Flammiger                               *
 *   github@netfg.net                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "vCard/vcard.h"
#include "vCard/strutils.h"
#include <vector>
#include <sys/stat.h>
#include "option.h"
#include "cardcurler.h"
#include "settings.h"
#include "version.h"
#include "url.h"
#include "cache.h"
#include "fileutils.h"
#include "searchtemplates.h"

void printError(const std::string &detail) {
    cout << detail << endl << endl;

    cout << "###############################" << endl;
    cout << "   **** " << APPNAME << " ****" << endl;
    cout << "###############################" << endl;
    cout << "          Version " << MAJOR << "." << MINOR << endl;
    cout << "*******************************" << endl << endl;

    cout << ":::: About ::::" << endl;
    cout << endl;
    cout << APPNAME << " is a small adressbook utility for the mutt mailclient to read your contacts" << endl;
    cout << "from an owncloud, SOGo, Xandikos, Davical carddav server. Licensed under GPLv2." << endl;
    cout << endl;
    cout << "(C) Torsten Flammiger, 2013" << endl;
    cout << "(C) Ike Devolder, 2015" << endl;
    cout << "(C) Torsten Flammiger, 2022..." << endl;
    cout << endl;
    cout << "See https://github.com/tfl/" << APPNAME << " for updates and source." << endl << endl;

    cout << ":::: Configure ::::" << endl;
    cout << endl;
    cout << "$ " << APPNAME << "  --name=GIVE-IT-A-NAME \\" << endl;
    cout << "                   --server=OWNCLOUD|SOGo-CARDDAV-URL|Davical|... \\" << endl;
    cout << "                   --username=USERNAME \\" << endl;
    cout << "                   --password=PASSWORD \\" << endl << endl;

    cout << ":::: Cache ::::" << endl;
    cout << endl;
    cout << "$ " << APPNAME << " --create-local-cache" << endl;
    cout << endl;
    cout << APPNAME << " will then create a local cache of all your vcards and will return data from" << endl;
    cout << "the cache first. If no data was found '" << APPNAME << "' will then query the server." << endl << endl;

    cout << ":::: Search ::::" << endl;
    cout << endl;
    cout << "$ " << APPNAME << " <query>" << endl;
    cout << endl;
    cout << "where <query> is part of the fullname or email to search. Dont use wildcards, like *" << endl << endl;

    cout << ":::: Notes ::::" << endl;
    cout << endl;
    cout << "- Enclose the parameter values in single or double quotes only if they contain whitespace" << endl;
    cout << endl;
    cout << "- Example resource urls:" << endl;
    cout << "  * Owncloud: http(s)://<YOUR SERVER.COM>/remote.php/carddav/addressbooks/<USERNAME>/<NAME OF YOUR LIST>" << endl;
    cout << "  * Davical: http(s)://<YOUR SERVER.COM>/caldav.php/<USERNAME>/addresses/" << endl;
    cout << endl;
    cout << "- The configuration is stored under ~/.config/" << APPNAME << endl << endl;
    cout << "- Your password will not be encrypted! Therefore a chmod go-rwx on ~/.config/" << APPNAME << endl;
    cout << "  will be executed everytime you configure the application" << endl;
    cout << endl;
    cout << "- Configuring the application with the same name will overwrite the existing entry" << endl;
    cout << endl;
}

int main(int argc, char *argv[])
{
    Settings cfg;
    Option opt(argc, argv, &cfg);
    std::string search = "";

    if(opt.doConfig()) {
        opt.configure();
        return 0;
    } else if (argc < 2) {
        printError("invalid or missing arguments");
        return 1;
    } else {
        // combine all the args in a space separated string.
        for(int i=1; i<argc; i++) {
            if(search != "") search += " ";
            search += argv[i];
        }
    }

    if(false == cfg.isValid()) {
        cout << "WARN: Invalid configuration or can't open '" << cfg.getConfigFile() << "'" << endl;
        cout << "WARN: Please create a valid configuration first.'" << endl;
        return 1;
    }

    // contains default search template strings for searching and exporting vcards
    SearchTemplates st;

    // preset search template
    bool doCache = false;

    std::string query = st.getDefaultSearchTemplate();
    if(opt.hasOption("--create-local-cache")) {
        query = st.getDefaultExportTemplate();

        // overwrite the default export template with one the user provided
        std::string templateFile = FileUtils::getHomeDir() + "/" + cfg.getConfigDir() + "/export.xml";
        if(FileUtils::fileExists(templateFile)) {
            if(false == FileUtils::getFileContent(templateFile, &query))
                return 1;
        }

        doCache = true;

    } else {
        // overwrite the default search template with one the user provided
        std::string templateFile = FileUtils::getHomeDir() + "/" + cfg.getConfigDir() + "/search.xml";
        if(FileUtils::fileExists(templateFile)) {
            if(false == FileUtils::getFileContent(templateFile, &query))
                return 1;
        }
    }

    // fetch the list of curlers (idea and partially code by Benjamin Frank <bfrank@net.t-labs.tu-berlin.de> on March 9, 2013)
    std::vector<std::string> sections = cfg.getSections();

    // there is the cache ;)
    std::string cachefile = cfg.getCacheFile();
    std::vector<Person> people;

    if(true == doCache) {
        if(FileUtils::fileExists(cachefile)) {
            if(FileUtils::fileRemove(cachefile)) {
                cout << "Old cache deleted" << endl;
            } else {
                cerr << "Failed to remove old cache database: " << cachefile << endl;
                return 1;
            }
        }

        for(std::vector<std::string>::iterator it = sections.begin(); it != sections.end(); ++it) {
            std::string section(*it);

            std::string server(cfg.getProperty(section, "server"));
            std::string url(Url::removePath(server));
            std::cout << "Creating cache entries for config section [" << section << "], URL: [" << server << "]" << std::endl;

            if(url.size() > 0) {
                CardCurler cc(cfg.getProperty(section, "username"), cfg.getProperty(section, "password"), server, argv[1]);
                std::vector<Person> tmp_people = cc.getAllCards(url, query);
                people.insert(people.end(), tmp_people.begin(), tmp_people.end());
            }
        }

        if(people.size() > 0 ) {
            Cache cache;
            if(false == cache.createDatabase())
                return 1;

            int numRecords = 0;
            std::cout << "Importing vcards" << std::endl;

            for(unsigned int i=0; i<people.size(); i++) {
                Person p = people.at(i);
                cache.addVCard(
                            p.FirstName,
                            p.LastName,
                            p.Emails,
                            p.rawCardData,
                            p.lastUpdatedAt
                );
                numRecords++;
            }

            chmod(cachefile.c_str(), S_IRUSR | S_IWUSR);
            cout << "Cache created (" << numRecords << " records)" << endl;
        } else {
            cout << "Export failed, nothing found" << endl;
        }
    } else {
        // 1. look into the cache
        bool cacheMiss = false;
        if(FileUtils::fileExists(cachefile)) {
            if(Option::isVerbose()) {
                std::cout << "Cache lookup in file " << cachefile;
            }
            
            people = CardCurler::curlCache(search);

            if(Option::isVerbose()) {
                std::cout << "Cache lookup returned " << people.size() << " records";
            }
        }

        // nothing found in cache? => search online
        if(people.size() == 0) {
           cacheMiss = true;
           StringUtils::replace(&query, "%s", search);

           // isn't it a nice duplication? - so get rid of it, stupid!
           for(std::vector<std::string>::const_iterator it = sections.begin(); it != sections.end(); ++it) {
                std::string section(*it);
                std::string server(cfg.getProperty(section, "server"));

              if(server.size() > 0) {
                   CardCurler cc(cfg.getProperty(section, "username"), cfg.getProperty(section, "password"), server, argv[1]);
                   std::vector<Person> tmp_people = cc.curlCard(query);
                   people.insert(people.end(), tmp_people.begin(), tmp_people.end());
              }
           }
        }

        if(people.size() > 0) {
            std::cout << std::endl; // needed by mutt to skip below the top line
            for(unsigned int i=0; i<people.size(); i++) {
                Person p = people.at(i);
                for(unsigned int j=0; j < p.Emails.size(); j++) {
                    std::cout << p.Emails.at(j) << '\t' << p.FirstName << ' ' << p.LastName << std::endl;
                }
            }

            // now update the cache
            if(cacheMiss && FileUtils::fileExists(cachefile)) {
                Cache cache;
                cache.openDatabase();
                for(unsigned int i=0; i<people.size(); i++) {
                    Person p = people.at(i);
                    cache.addVCard(p.FirstName, p.LastName, p.Emails, p.rawCardData, p.lastUpdatedAt);
                }
            }

        } else {
            cout << "Search returned no results" << endl;
        }
    }

    return 0;
}
