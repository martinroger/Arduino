#include <Arduino.h>

HardwareSerial MINISerial(1);
HardwareSerial IPODSerial(2);

unsigned long lastRefresh = 0;

class sumChecker
{
private:
  Stream& _serialToListen;
  char* _name;
  unsigned long _rxLast_ts = 0;

//Packet-related 
  byte _prevRxByte    =   0x00;
  byte _rxBuf[1024]   =   {0x00};
  uint32_t _rxLen     =   0;
  uint32_t _rxCounter =   0;
  bool _rxInProgress  =   false;
public:
  sumChecker(Stream& targetStream, const char* name);
  ~sumChecker();

  static byte checksum(const byte* byteArray, uint32_t len);
  byte refresh();
  void processPacket(const byte* byteArray,uint32_t len);
};

sumChecker::sumChecker(Stream& targetStream, const char* name) : _serialToListen(targetStream),_name((char*)name)
{
	ESP_LOGD(name,"SumChecker %s created",_name);
}

sumChecker::~sumChecker()
{
}

byte sumChecker::checksum(const byte *byteArray, uint32_t len)
{
    uint32_t tempChecksum = len;
    for (int i=0;i<len;i++) {
        tempChecksum += byteArray[i];
    }
    tempChecksum = 0x100 -(tempChecksum & 0xFF);
    return (byte)tempChecksum;
}

byte sumChecker::refresh()
{
	//ESP_LOGD(_name,"Refresh called");
	byte ret = 0xFF; //Abnormal
	while(_serialToListen.available()) 
	{
		byte incomingByte = _serialToListen.read();
		_rxLast_ts = millis();
		//A new 0xFF55 packet starter shows up
		if(_prevRxByte == 0xFF && incomingByte == 0x55 && !_rxInProgress) 
		{ 
			//ESP_LOGD(_name,"Packet start");
			_rxLen = 0; //Reset the received length
			_rxCounter = 0; //Reset the counter to the end of payload
			_rxInProgress = true;
		}
		else if(_rxInProgress)
		{
			if(_rxLen == 0 && _rxCounter == 0)
			{
				_rxLen = incomingByte;
			}
			else
			{
				_rxBuf[_rxCounter++] = incomingByte;
				if(_rxCounter == _rxLen+1) //We are done receiving the packet
				{ 
					_rxInProgress = false;
					byte tempChecksum = sumChecker::checksum(_rxBuf, _rxLen);
					if (tempChecksum == _rxBuf[_rxLen]) //Checksum checks out
					{ 
						processPacket(_rxBuf,_rxLen);  
					}
					else
					{
						//Do something if it does not check out at the checksum
						ESP_LOGW(_name,"Checksum error exp %02x rcv %02x",tempChecksum,_rxBuf[_rxLen]);
						ESP_LOG_BUFFER_HEXDUMP(_name,_rxBuf,_rxLen+1,ESP_LOG_ERROR);
						ret = 0x01;
					}
				}
			}
		}
		else if(!_rxInProgress && incomingByte!=0xFF)
		{
			ESP_LOGW(_name,"Unexpected Serial byte: %02x",incomingByte);
			ret = 0x02;
		}
		_prevRxByte = incomingByte;
	}
	if((millis()-_rxLast_ts > 1000) && _rxInProgress)
	{
		ESP_LOGW(_name,"Incomplete RX : received %d expected %d",_rxCounter,_rxLen+1);
		_rxInProgress = false;
		ret = 0x03;
	}

	return ret;

}


void sumChecker::processPacket(const byte *byteArray, uint32_t len)
{
	//Does nothing
}


//------------------------------------------------------------------------------

sumChecker MINICheck(MINISerial,"MINI");
sumChecker IPODCheck(IPODSerial,"IPOD");
byte ipod = 0xFF;
byte mini = 0xFF;



void setup() {
  rgbLedWrite(21,0,0,0);
//   delay(1000);
//   rgbLedWrite(21,128,0,0);
//   delay(1000);
//   rgbLedWrite(21,0,128,0);
//   delay(1000);
//   rgbLedWrite(21,0,0,128);
//   delay(1000);
//   rgbLedWrite(21,0,0,0);
//   delay(1000);
  Serial.begin(115200);
  MINISerial.setPins(7,8);
  IPODSerial.setPins(6,5);
  MINISerial.begin(19200);
  IPODSerial.begin(19200);
  while (MINISerial.available())
  {
    MINISerial.read();
  }
  while(IPODSerial.available())
  {
    IPODSerial.read();
  }
  pinMode(0,INPUT);
  ESP_LOGI("Setup","Ready to loop()");
}

void loop() {
	if(millis()-lastRefresh>5)
	{
		lastRefresh = millis();
		mini = MINICheck.refresh();
		ipod = IPODCheck.refresh();
		if((mini!=0xFF) || (ipod!=0xFF))
		{
			rgbLedWrite(21, 255*((mini==0x01)||(ipod==0x01)), 255*((mini==0x02)||(ipod==0x02)), 255*((mini==0x03)||(ipod==0x03)));
		}
		if(!digitalRead(0))
		{
			rgbLedWrite(21,0,0,0);
			ESP_LOGD("FlagReset","Resetting LED");
		}
	}

}

