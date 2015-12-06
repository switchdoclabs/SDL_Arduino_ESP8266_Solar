
// SAP Buffer Routines

void initSAPBuffer()
{
  CurrentSAPBuffer = -1;

  lastReadSAPBuffer = -1;
  int i;
  for (i = 0; i < SAPBUFFERSIZE; i++)
  {

    SAPBuffer[i].timeStamp = 0;


  }

}



void writeSAPBuffer()
{

  //Serial.print("Entry WriteSAPBuffer C / R ");
  //Serial.print(CurrentSAPBuffer);
  //Serial.print(" / ");
  //Serial.println(lastReadSAPBuffer);
  CurrentSAPBuffer++;

  if (CurrentSAPBuffer >=  SAPBUFFERSIZE)  // wrap around
  {
    CurrentSAPBuffer = 0;

  }


  SAPBuffer[CurrentSAPBuffer].timeStamp = millis();
  SAPBuffer[CurrentSAPBuffer].SAPEntry = currentSAPData;
  //Serial.print("Exit WriteSAPBuffer C / R ");
  //Serial.print(CurrentSAPBuffer);
  //Serial.print(" / ");
  //Serial.println(lastReadSAPBuffer);


}

int returnCountSAPBuffer()
{

  int i;
  int count = 0;
  for (i = 0; i < SAPBUFFERSIZE; i++)
  {
    if (SAPBuffer[i].timeStamp > 0)
      count++;
  }

  return count;

}



int readSAPBuffer(SAPBufferStruct *mySAPBuffer)
{
  //Serial.print("inReadSAPBuffer C / R ");
  //Serial.print(CurrentSAPBuffer);
  //Serial.print(" / ");
  //Serial.println(lastReadSAPBuffer);


  // read out all data, doesn't matter the order.   The SQL Databoase on the Pi will figure it out...

  if (lastReadSAPBuffer == -1 ) // deal with first read no matter how long ago
  {
    // scan Buffer for smallest > 0 timeStamp - set to lastReadSAPBuffer
    int i;

    int smallestTimeStamp = 4294967295;  // 2^32-1
    int smallestIndex = 0;

    for (i = 0; i < SAPBUFFERSIZE;  i++)
    {
      if (SAPBuffer[i].timeStamp != 0)
      {
        if (SAPBuffer[i].timeStamp < smallestTimeStamp)
        {
          smallestTimeStamp = SAPBuffer[i].timeStamp;
          smallestIndex = i;
        }
      }


    }
    lastReadSAPBuffer = smallestIndex;
  }

  if (SAPBuffer[lastReadSAPBuffer].timeStamp  == 0)
  {
    return -1;
  }


  *mySAPBuffer = SAPBuffer[lastReadSAPBuffer];
  SAPBuffer[lastReadSAPBuffer].timeStamp = 0;


  lastReadSAPBuffer++;
  if (lastReadSAPBuffer >= SAPBUFFERSIZE)
  {
    lastReadSAPBuffer = 0;

  }

}


String assembleSAPBuffer()
{
  int status;
  SAPBufferStruct mySAPBuffer;

  String returnString;
  returnString = "";


  returnString = String(ESP.getFreeHeap());

  status = readSAPBuffer(&mySAPBuffer);
  while (status != -1)
  {
    if (returnString.length() != 0)
    {
      returnString += " | ";
    }

    String sensorBuild;
    sensorBuild = String(mySAPBuffer.timeStamp) + ",";

    int i;
    for (i = 0; i < 3; i++)
    {

      sensorBuild += String(mySAPBuffer.SAPEntry.busVoltage[i], 2) + ",";
      sensorBuild += String(mySAPBuffer.SAPEntry.loadVoltage[i], 2) + ",";
      if (i < 2)
        sensorBuild += String(mySAPBuffer.SAPEntry.current[i], 2) + ",";
      else
        sensorBuild += String(mySAPBuffer.SAPEntry.current[i], 2) ;

      //Serial.print("sensorBuild=");
      //Serial.println(sensorBuild);
    }
    returnString += sensorBuild;
    status = readSAPBuffer(&mySAPBuffer);
  }
  return returnString;




}



void printDebugFullSAPBuffer()
{

  Serial.print("DebugFullSAPBuffer State C / R ");
  Serial.print(CurrentSAPBuffer);
  Serial.print(" / ");
  Serial.println(lastReadSAPBuffer);
  int i;
  for (i = 0; i < SAPBUFFERSIZE; i++)
  {

    Serial.print("index:");
    Serial.print(i);
    Serial.print(" timeStamp = ");
    Serial.print(SAPBuffer[i].timeStamp);

    if (SAPBuffer[i].timeStamp == 0)
    {
      Serial.println(" SAPEntry = null");
    }
    else
    {

      Serial.println(" SAPEntry = Full");
      /*
        SAPData currentSAPData;

        currentSAPData = SAPBuffer[i].SAPEntry;

        Serial.println("--------SAP ENTRY-------");
        Serial.print("LIPO_Battery Current:       "); Serial.print(currentSAPData.current[0]); Serial.println(" mA");




        Serial.print("Solar Cell Bus Voltage:   "); Serial.print(currentSAPData.busVoltage[1]); Serial.println(" V");


        Serial.print("Output Bus Current:       "); Serial.print(currentSAPData.current[2]); Serial.println(" mA");

      */

    }


  }


}



// Read data from specific SunAirPlus unit (SAP0 - SAP2)

void   startSAPINA3221()
{

  ina3221_SAP.begin();  // SAP

}

void readSAP()
{




  int i;
  for (i = 0; i < 3; i++)
  {
    currentSAPData.busVoltage[i] = 0.0f;
    currentSAPData.current[i] = 0.0f;
    currentSAPData.loadVoltage[i] = 0.0f;
  }



  currentSAPData.busVoltage[0] = ina3221_SAP.getBusVoltage_V(SAP_LIPO_BATTERY_CHANNEL + 1);
  currentSAPData.current[0] = ina3221_SAP.getCurrent_mA(SAP_LIPO_BATTERY_CHANNEL + 1); // minus is to get the "sense" right.   - means the battery is charging, + that it is discharging
  currentSAPData.loadVoltage[0] = currentSAPData.busVoltage[0] + (ina3221_SAP.getShuntVoltage_mV(SAP_LIPO_BATTERY_CHANNEL + 1) / 1000);

  currentSAPData.busVoltage[1] = ina3221_SAP.getBusVoltage_V(SAP_SOLAR_CELL_CHANNEL + 1);
  currentSAPData.current[1] = - ina3221_SAP.getCurrent_mA(SAP_SOLAR_CELL_CHANNEL + 1); // minus is to get the "sense" right.   - means the battery is charging, + that it is discharging
  currentSAPData.loadVoltage[1] = currentSAPData.busVoltage[1] + (ina3221_SAP.getShuntVoltage_mV(SAP_SOLAR_CELL_CHANNEL + 1) / 1000);

  currentSAPData.busVoltage[2] = ina3221_SAP.getBusVoltage_V(SAP_OUTPUT_CHANNEL + 1);
  currentSAPData.current[2] = ina3221_SAP.getCurrent_mA(SAP_OUTPUT_CHANNEL + 1); // minus is to get the "sense" right.   - means the battery is charging, + that it is discharging
  currentSAPData.loadVoltage[2] = currentSAPData.busVoltage[2] + (ina3221_SAP.getShuntVoltage_mV(SAP_OUTPUT_CHANNEL + 1) / 1000);

  Serial.println("--------SAP Data-------");

  Serial.print("LIPO_Battery Bus Voltage:   "); Serial.print(currentSAPData.busVoltage[0]); Serial.println(" V");
  Serial.print("LIPO_Battery Load Voltage:  "); Serial.print(currentSAPData.loadVoltage[0]); Serial.println(" V");
  Serial.print("LIPO_Battery Current:       "); Serial.print(currentSAPData.current[0]); Serial.println(" mA");
  Serial.println("");



  Serial.print("Solar Cell Bus Voltage:   "); Serial.print(currentSAPData.busVoltage[1]); Serial.println(" V");
  Serial.print("Solar Cell Load Voltage:  "); Serial.print(currentSAPData.loadVoltage[1]); Serial.println(" V");
  Serial.print("Solar Cell Current:       "); Serial.print(currentSAPData.current[1]); Serial.println(" mA");
  Serial.println("");


  Serial.print("Output Bus Bus Voltage:   "); Serial.print(currentSAPData.busVoltage[2]); Serial.println(" V");
  Serial.print("Output Bus Load Voltage:  "); Serial.print(currentSAPData.loadVoltage[2]); Serial.println(" V");
  Serial.print("Output Bus Current:       "); Serial.print(currentSAPData.current[2]); Serial.println(" mA");
  Serial.println("");
  Serial.println("--------");

  return;
}


