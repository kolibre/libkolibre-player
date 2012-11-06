/*
Copyright (C) 2012 Kolibre

This file is part of kolibre-player.

Kolibre-player is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Kolibre-player is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with kolibre-player. If not, see <http://www.gnu.org/licenses/>.
*/


/*
 * Daisy time code parser
 */

#ifndef DAISYTIMECODE_H_
#define DAISYTIMECODE_H_

#include <sstream>
#include <iostream>
#include <algorithm>
#include <locale.h>
#include <cstring>

//Begins the implementation of SMIL timing :
// http://www.w3.org/TR/2005/REC-SMIL2-20050107/smil-timing.html#Timing-LanguageDefinition

#define MAXBUFLEN 128

/** This class loads a TimeCode structure with the correct
  beginning and end. A SMIL type of string is used as input for
  both times
  */
class SmilTimeCode {
    private:
        unsigned long start, end;

        //This function performs the raw string to milliseconds conversion
        //Input is a SMIL compliant string
        //Output is a millisecond count.
        unsigned long convertStrToTimeCode(const char *strSMIL){
            char * timecode = strdup(strSMIL);
            int startMetric=0;

            if(localeconv()->decimal_point[0] != '.') {
                for(unsigned i = 0; i < strlen(timecode); i++)
                    if(timecode[i] == '.')
                        timecode[i] = localeconv()->decimal_point[0];
            }



            // Check if we have a unit, or how many :'s there is

            char *pos;
            if ((pos = strstr(timecode,"ms")) != NULL){
                //we are receiving milliseconds
                *pos = '\0'; //Mark a new end to the string
                startMetric = 0; //Indicate the start metric

            } else if ((pos=strstr(timecode,"s"))!=NULL){
                //we are receiving seconds
                *pos = '\0'; //Mark a new end to the string
                startMetric = 1; //Indicate the start metric

            } else if ((pos=strstr(timecode,"min"))!=NULL){
                //we are receiving minutes
                *pos = '\0'; //Mark a new end to the string
                startMetric = 2; //Indicate the start metric

            } else if ((pos = strstr(timecode, "h"))!=NULL){
                //we are receiving hours
                *pos = '\0'; //Mark a new end to the string
                startMetric = 3; //Indicate the start metric

            } else {
                //No specification of unit
                // Check number of ':' in the string
                int cnt=0;
                for (unsigned int i=0;i<strlen(timecode);i++){
                    if (timecode[i]==':')
                        cnt++;
                }

                if (cnt==3)//We are using hours minutes seconds milliseconds
                    startMetric=3;
                else if (cnt==2)//We are using hours minutes seconds
                    startMetric=3;
                else if (cnt==1)//We are using minutes seconds
                    startMetric=2;
                else //We are using seconds
                    startMetric=1;
            }

            /*
            std::cout << "Largest unit mentioned = ";
            switch(startMetric)
            {
                case 0: std::cout << "ms";
                    break;
                case 1: std::cout << "s";
                    break;
                case 2: std::cout << "min";
                    break;
                case 3: std::cout << "h";
                    break;
            }
            std::cout << std::endl;
            */

            pos = timecode;
            char *vptr = timecode;
            int finished = 0;

            double fields[4] = { 0, 0, 0, 0 };

            // Loop trough the string catching all the values and converting them to a float
            while (startMetric >= 0 && !finished) {
                if(*pos == ':' || *pos == '\0') {
                    if(*pos == '\0') finished = 1;
                    // Convert the : into an end of string
                    *pos = '\0';

                    // save and decrement the metric value
                    fields[startMetric--] = atof(vptr);
                    //std::cout << "Got:" << atof(vptr) << std::endl;

                    pos++;
                    vptr = pos;
                }
                pos++;
            }

            /*
            std::cout << fields[3] << "h\t"
                << fields[2] << "m\t"
                << fields[1] << "s\t"
                << fields[0] << "ms" <<std::endl;
            */

            fields[0] += fields[3] * 60 * 60 * 1000;
            fields[0] += fields[2] * 60 * 1000;
            fields[0] += fields[1] * 1000;

            free(timecode);

            return (unsigned long) fields[0];

        }

    public:
        //The time code is restricted to MAX_INT hours, 60 minutes, 60 seconds and 1000 milliseconds
        SmilTimeCode(const char *SMILBegin, const char *SMILEnd) {
            start = 0;
            end = 0;


            //Set the begin time
            if(strlen(SMILBegin) > 0) {
                if(strstr(SMILBegin, "npt=") == NULL)
                    start = convertStrToTimeCode(SMILBegin);
                else
                    start = convertStrToTimeCode((SMILBegin+4));
            }

            //Set the end of time
            if(strlen(SMILEnd) > 0) {
                if(strstr(SMILEnd, "npt=") == NULL)
                    end = convertStrToTimeCode(SMILEnd);
                else
                    end = convertStrToTimeCode((SMILEnd+4));
            }
        }

        unsigned long getStart() {
            return start;
        }
        unsigned long getEnd() {
            return end;
        }
};

#endif
