/*

(C) Copyright 2012-2013 Massachusetts Institute of Technology

This file is part of the GSMExtras library.

The GSMExtras library is free software: you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 2.1 of the
License, or (at your option) any later version.

The GSMExtras library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GSMExtras library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "PhoneBook.h"
#include <Arduino.h>

const int READPHONEBOOK = 100;
const int WRITEPHONEBOOK = 101;
const int QUERYPHONEBOOK = 102;
const int SELECTPHONEBOOK = 103;

PROGMEM const char _command_CPBR[]={"AT+CPBR="};
PROGMEM const char _command_CPBW[]={"AT+CPBW="};
PROGMEM const char _command_CPBS[]={"AT+CPBS"};

char *_phoneBookTypes[] = { "SM", "MC", "RC", "DC", "AC", "ON", "ME" };

PhoneBook::PhoneBook()
{
  
};

void PhoneBook::selectPhoneBook(int type)
{
  phoneBookType = type;
  theGSM3ShieldV1ModemCore.openCommand(this,(GSM3_commandType_e)SELECTPHONEBOOK);
  selectPhoneBookContinue();
  //while (!ready());
}

void PhoneBook::selectPhoneBookContinue()
{
  bool resp;
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(_command_CPBS, false);
      theGSM3ShieldV1ModemCore.print("=\"");
      theGSM3ShieldV1ModemCore.print(_phoneBookTypes[phoneBookType]);
      theGSM3ShieldV1ModemCore.print("\"\r");
    case 2:
      if(theGSM3ShieldV1ModemCore.genericParse_rsp(resp))
      {
        if (resp) theGSM3ShieldV1ModemCore.closeCommand(1);
        else theGSM3ShieldV1ModemCore.closeCommand(3);
      }
      break;
  }
}

int PhoneBook::getPhoneBookType()
{
  //queryPhoneBook();
  return phoneBookType;
}

int PhoneBook::getPhoneBookSize()
{
  //queryPhoneBook();
  return phoneBookSize;
}

int PhoneBook::getPhoneBookUsed()
{
  //queryPhoneBook();
  return phoneBookUsed;
}

void PhoneBook::queryPhoneBook()
{
  theGSM3ShieldV1ModemCore.openCommand(this,(GSM3_commandType_e)QUERYPHONEBOOK);
  queryPhoneBookContinue();
  //while (!ready());
}

void PhoneBook::queryPhoneBookContinue()
{
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(_command_CPBS, false);
      theGSM3ShieldV1ModemCore.print("?\r");
      break;
    case 2:
      if (parseCPBS()) theGSM3ShieldV1ModemCore.closeCommand(1);
      else theGSM3ShieldV1ModemCore.closeCommand(3);
      break;
  }  
}

bool PhoneBook::parseCPBS()
{
  char buf[3];
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil("+CPBS: ", true)) return false;
  if (!theGSM3ShieldV1ModemCore.theBuffer().extractSubstring("\"", "\"", buf, sizeof(buf))) return false;
  
  phoneBookType = PHONEBOOK_UNKNOWN;
  
  for (int i = 0; i < sizeof(_phoneBookTypes) / sizeof(_phoneBookTypes[0]); i++)
    if (!strcmp(buf, _phoneBookTypes[i]))
      phoneBookType = i;

  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", false)) return false;
  
  phoneBookUsed = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  
  theGSM3ShieldV1ModemCore.theBuffer().read(); // skip previous ","
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", false)) return false;

  phoneBookSize = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  
  return true;
}

void PhoneBook::writePhoneBookEntry(int index, char number[], char name[])
{
  phoneBookIndex = index;
  strncpy(this->number, number, 20);
  this->number[19] = 0;
  strncpy(this->name, name, 20);
  this->name[19] = 0;
  theGSM3ShieldV1ModemCore.openCommand(this,(GSM3_commandType_e)WRITEPHONEBOOK);
  writePhoneBookContinue();
}

void PhoneBook::addPhoneBookEntry(char number[], char name[])
{
  writePhoneBookEntry(0, number, name);
}

void PhoneBook::deletePhoneBookEntry(int index)
{
  writePhoneBookEntry(index, "", "");
}

void PhoneBook::writePhoneBookContinue()
{
  bool resp;
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(_command_CPBW, false);
      if (phoneBookIndex != 0) theGSM3ShieldV1ModemCore.print(phoneBookIndex);
      if (strlen(number) != 0 || strlen(name) != 0) {
        theGSM3ShieldV1ModemCore.print(",\"");
        theGSM3ShieldV1ModemCore.print(number);
        theGSM3ShieldV1ModemCore.print("\",129,\""); // 129 = unknown number type
        theGSM3ShieldV1ModemCore.print(name);
        theGSM3ShieldV1ModemCore.print("\"");
      }
      theGSM3ShieldV1ModemCore.print("\r");
      break;
    case 2:
      if(theGSM3ShieldV1ModemCore.genericParse_rsp(resp))
      {
        if (resp) theGSM3ShieldV1ModemCore.closeCommand(1);
        else theGSM3ShieldV1ModemCore.closeCommand(3);
      }
      break;
  }
}

void PhoneBook::readPhoneBookEntry(int index)
{
  phoneBookIndex = index;
  gotNumber = false; gotTime = false;
  theGSM3ShieldV1ModemCore.openCommand(this,(GSM3_commandType_e)READPHONEBOOK);
  readPhoneBookContinue();
}

void PhoneBook::readPhoneBookContinue()
{
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(_command_CPBR, false);
      theGSM3ShieldV1ModemCore.print(phoneBookIndex);
      theGSM3ShieldV1ModemCore.print("\r");
      break;
    case 2:
      if (parseCPBR()) theGSM3ShieldV1ModemCore.closeCommand(1);
      else theGSM3ShieldV1ModemCore.closeCommand(4);
      break;
  }
}

bool PhoneBook::parseCPBR()
{
  char c;
  int i;
  
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil("+CPBR: ", true)) return false;
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",\"", true)) return true;
  gotNumber = true;
  
  i = 0;
  while ((c = theGSM3ShieldV1ModemCore.theBuffer().read()) != 0) {
    if (c == '"') break;
    if (i < PHONEBOOK_BUFLEN - 1) number[i++] = c;
  }
  number[i] = 0;
  
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",\"", true)) return true;
  
  i = 0;
  while ((c = theGSM3ShieldV1ModemCore.theBuffer().read()) != 0) {
    if (c == '"') break;
    if (i < PHONEBOOK_BUFLEN - 1) name[i++] = c;
  }
  name[i] = 0;
  
  if (!theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", true)) return true;
  
  gotTime = true;
  
  datetime.year = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
  datetime.month = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous '/'
  theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
  datetime.day = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", false);
  datetime.hour = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
  datetime.minute = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous ':'
  theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
  datetime.second = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	
  return true;
}

void PhoneBook::manageResponse(byte from, byte to)
{
  switch(theGSM3ShieldV1ModemCore.getOngoingCommand())
  {
    case NONE:
      theGSM3ShieldV1ModemCore.gss.cb.deleteToTheEnd(from);
      break;
    case READPHONEBOOK:
      readPhoneBookContinue();
      break;
    case WRITEPHONEBOOK:
      writePhoneBookContinue();
      break;
    case QUERYPHONEBOOK:
      queryPhoneBookContinue();
      break;
  }
}
