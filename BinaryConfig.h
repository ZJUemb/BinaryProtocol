#ifndef BINARY_CONFIG_H
#define BINARY_CONFIG_H


#define DataLenMax	16
#define SensorMax	10
typedef enum{
	DataType_NONE,
	DataType_Int,
	DataType_Float,
	DataType_String,
}DataType;
typedef struct{
	const char* dataName;
	DataType type;
	int length;
}DataItem;
typedef struct{
	int device_id;
	int dataCount;
	DataItem dataConfigs[DataLenMax];
}SensorConfig;
typedef enum{
	Sensor_872 = 0,
	Sensor_875 = 1,
	Sensor_870 = 2,
	Sensor_883 = 3
}SensorID;
//Copy From Server
typedef enum{
	DeviceID_872 = 872,
	DeviceID_875 = 875,
	DeviceID_870 = 870,
	DeviceID_883 = 883,
}DeviceID;

SensorConfig binaryReportConfigs[SensorMax] = {
	{ //Sensor_872
		DeviceID_872,
		5,			
		"type",		DataType_Int,	4,
		"temp",		DataType_Float, 4,
		"wet",		DataType_Float, 4,
		"state",	DataType_Int,	4,
		"id",		DataType_Int,	4
	},
	{ //Sensor_875
		DeviceID_875,//device_id
		4,			//dataCount
		"type",		DataType_Int,	4,
		"pid",		DataType_Int,	4,
		"time",		DataType_Int,	4,
		"result",	DataType_Int,	4
	},
	{ //Sensor_870
		DeviceID_870,
		2,			
		"type",		DataType_Int,	4,
		"state",	DataType_Int,	4,
	},
	{ //Sensor_883
		DeviceID_883,
		2,			
		"state",	DataType_Int,	4,
		"value",	DataType_Float,	4,
	},
};

SensorConfig binaryCtrlConfigs[SensorMax] = {
	{ //Sensor_872
		DeviceID_872,
		1,			
		"start",	DataType_Int,	4
	},
	{ //Sensor_875
		DeviceID_875,
		1,
		"result",	DataType_Int,	4
	},
	{ //Sensor_870
		DeviceID_870,
		1,
		"action",	DataType_Int,	4
	},
	{ //Sensor_883
		DeviceID_883,
		1,
		"action",	DataType_Int,	4
	},
};

SensorConfig* GuessReportSensor(int device_id){
	for (int i = 0; i < SensorMax; i++)
		if (binaryReportConfigs[i].device_id == device_id)
			return binaryReportConfigs + i;
	return NULL;
}
SensorConfig* GuessCtrlSensor(int device_id){
	for (int i = 0; i < SensorMax; i++)
		if (binaryCtrlConfigs[i].device_id == device_id)
			return binaryCtrlConfigs + i;
	return NULL;
}
#endif