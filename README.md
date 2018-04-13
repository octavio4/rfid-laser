# rfid-laser
control acces for a laser machine in a makerspace.
There original 40w laser was replaced by a longer 60w laser tube,so a MFD box was build to accomodate it.I put the electronics inside this box whith aluminium shield to protect from the electrostatics fields of the laser tube,but this is not enought and sometimes it fails.So it is better to put this device elsewere. 
RFIDLASE.PNG  is the shematic.
*.ino is the arduino program.The source code has a few lines of code to erase the eeprom.The first time these lines must be uncommented and the program must be run one time to erase the eeprom,then this code must be commented,compiled and uploaded again to the arduino board.Two master rfids must be readed the first time ,wich will be used to manage the other rfids.The program will show instructions about usage on the lcd.Time start counting as soon as the laser is fired,so it will not work if the current sensor is not connected.
