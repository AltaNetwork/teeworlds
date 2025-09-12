### ⚠️ *This project migrated to [AltaSport](https://github.com/AltaNetwork)*⚠️
# LonerStuff
This is a multi gamemode modification with accounts ( *built in Serbia by veqi* )

# Building the server and installing dependencies
    # Debian/Ubuntu
    sudo apt install build-essential gcc python3 pkg-config zlib1g-dev libicu-dev libmysqlclient-dev libmysqlcppconn-dev bam

    # Void Linux
    sudo xbps-install -S base-devel gcc python3 pkg-config zlib-devel icu-devel libmysqlclient-devel mysql++-devel

    # Arch Linux
    sudo pacman -S base-devel gcc python pkg-config zlib icu mariadb-libs mysql++
    
    # To build the server just run "bam" in main folder
    bam

# Project structure
    teeworlds_srv
    autoexec.cfg
    data/
      maps/
        *maps*
      languages/
        index.json
        *and then languages*
### Example of an *index.json* file
    {
        "language indices":
        [
            {
              "file": "en",
              "name": "english"
            },
            {
              "file": "de",
              "name": "German",
              "parent": "en"
            },
            {
              "file": "rs",
              "name": "Српски",
              "parent": "en"
            }
        ]
    }
### Example of language file ( *Used rs.json cut as example* )
    {
      "translation": [
        {
          "key": "Language successfully switched to English",
          "value": "Језик је успешно пребачен на Српски"
        },
        {
          "key": "Unknown language or no input language code",
          "value": "Непознат језик или није унет код језика"
        },
        {
          "key": "Available languages: {str:ListOfLanguage}",
          "value": "Доступни језици: {str:ListOfLanguage}"
        },
        {
          "key": "'{str:PlayerName}' entered and joined the game",
          "value": "'{str:PlayerName}' је ушао и придружио се игри"
        },
        {
          "key": "'{str:PlayerName}' has left the game",
          "value": "'{str:PlayerName}' је напустио игру"
        }
      ]
    }



# License

    Teeworlds Copyright (C) 2007-2014 Magnus Auvinen

    DDRace    Copyright (C) 2010-2011 Shereef Marzouk

    DDNet     Copyright (C) 2013-2015 Dennis Felsing

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.*

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.*

    IMPORTANT NOTE! The source under src/engine/external are stripped
    libraries with their own licenses. Mostly BSD or zlib/libpng license but
    check the individual libraries.