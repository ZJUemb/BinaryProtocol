#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define OS_LINUX
#include <stdio.h>

#include <string.h>
#include <stdlib.h>


#ifdef OS_LINUX
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <sys/shm.h>
#else
	#include <winsock2.h>
#endif
#include "cJSON.h"
#include "BinaryProtocol.h"
#include "BinaryConfig.h"



void ReceivePacket(int sockfd, char *packet, int size){
	for (int count = 0, ret; count < size; count += ret)
		if ((ret = recv(sockfd, packet + count, size - count, 0)) == SOCKET_ERROR){
			printf("Error in ReceivePacket!");
			closesocket(sockfd);
			exit(1);
		}
		else if (!ret)
			return;
}
int ParseJson(char *dataStr, SensorConfig *dataStructure, char *payload){
	if (dataStructure == NULL)
		return -1;
	cJSON *jsonRoot, *jsonSub;
	int i;
	if (dataStr == NULL)
		return -1;
	jsonRoot = cJSON_Parse(dataStr);
	if (jsonRoot == NULL)
		return -1;
	for (i = 0; i<dataStructure->dataCount; i++){
		jsonSub = cJSON_GetObjectItem(jsonRoot, dataStructure->dataConfigs[i].dataName);
		if (jsonSub == NULL)
			return -1;
		switch (dataStructure->dataConfigs[i].type){
		case DataType_Int:
			*(int *)payload = jsonSub->valueint;
			break;
		case DataType_Float:
			*(float *)payload = jsonSub->valuedouble;
			break;
		case DataType_String:
			strcpy(payload, jsonSub->valuestring);
			break;
		default:
			return -1;
		}
		payload += dataStructure->dataConfigs[i].length;
	}
	return 1;
}
int ParseBinary(char *payload, SensorConfig *dataStructure, char *dataJson){
	if (dataStructure == NULL)
		return -1;
	cJSON *root;
	root = cJSON_CreateObject();
	for (int i = 0; i < dataStructure->dataCount; i++){
		switch (dataStructure->dataConfigs[i].type){
		case DataType_Int:
			cJSON_AddNumberToObject(root, dataStructure->dataConfigs[i].dataName, *(int *)payload);
			break;
		case DataType_Float:
			cJSON_AddNumberToObject(root, dataStructure->dataConfigs[i].dataName, *(float *)payload);
			break;
		case DataType_String:
			cJSON_AddStringToObject(root, dataStructure->dataConfigs[i].dataName, payload);
			break;
		default:
			return -1;
		}
		payload += dataStructure->dataConfigs[i].length;
	}
	strcpy(dataJson,cJSON_Print(root));
	return 1;
}


int Login(int sockfd, int device_id, char *auth_key){
	LoginPacket login;
	AckPacket	ack;
	int			ret;
	login.msgType = MSG_LOGIN;
	login.deviceId = device_id;
	memset(login.key, 0, sizeof(login.key));
	strcpy(login.key, auth_key);
	send(sockfd, (char*)&login, sizeof(LoginPacket), 0);
	ret = recv(sockfd, (char*)&ack, sizeof(AckPacket), 0);
	if (ret < 0 || ack.msgType == MSG_NAK)
		return -1;
	return 1;
}

void Logout(int sockfd){
	LogoutPacket logout;
	logout.msgType = MSG_LOGOUT;
	send(sockfd, (char*)&logout, sizeof(LogoutPacket), 0);
}

int Report(int sockfd, int device_id, char *jsonData){
	int ret;
	ReportPacket report;
	AckPacket ack;
	report.msgType = MSG_REPORT;
	report.device_id = device_id;
	memset(report.payload, 0, sizeof(report.payload));
	ret = ParseJson(jsonData, GuessReportSensor(device_id), report.payload);
	if (ret <= 0)
		return ret;
	ret = send(sockfd, (char*)&report, sizeof(ReportPacket), 0);
	if (ret < 0)
		return -1;
	ret = recv(sockfd, (char*)&ack, sizeof(AckPacket), 0);
	if (ret < 0 || ack.msgType == MSG_NAK)
		return -1;
	return 1;
}
int Ctrl(int sockfd, int *device_id, char *dataJson){
	CtrlPacket ctrl;
	int ret;
	//ReceivePacket(sockfd, (char*)&ctrl, sizeof(CtrlPacket));
	
	ret = recv(sockfd, (char*)&ctrl, sizeof(CtrlPacket), 0);
	//printf("%d\n", ret);
	//ret = recv(sockfd, (char*)&ctrl, sizeof(CtrlPacket), 0);
	if (ret <= 0 || ctrl.msgType != MSG_CTRL)
		return -1;
	*device_id = ctrl.device_id;
	ret = ParseBinary(ctrl.payload, GuessCtrlSensor(ctrl.device_id), dataJson);
	if (ret < 0)
		return -1;
	return 1;
}


int Init(){

#ifdef OS_LINUX
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(10659);  ///服务器端口
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");  ///服务器ip
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		exit(1);
	}
#else
	WSADATA wsaData;
	struct hostent *he;
	struct sockaddr_in sockAddr;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 1;
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == INVALID_SOCKET){
		WSACleanup();
		exit(1);
	}
	if ((he = gethostbyname("106.2.111.110")) == NULL){
		WSACleanup();
		exit(1);
	}
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(10659);
	sockAddr.sin_addr = *(struct in_addr *)he->h_addr_list[0];
	memset(&(sockAddr.sin_zero), 0, sizeof(sockAddr.sin_zero));
	if (connect(sockfd, (struct sockaddr *)&sockAddr, sizeof(struct sockaddr)) == SOCKET_ERROR){
		closesocket(sockfd);
		WSACleanup();
		exit(1);
	}
#endif
	return sockfd;
}
int main(){

	
	int ret;
	int sockfd = Init();
	
	/*test*/
	
	char *testdatajson = "{\"type\":123,\"temp\":2,\"wet\":3,\"state\":1,\"id\":12}";
	int testGateID = 7;
	char *testGateKey = "17fa24fbc85ad56c15bf415aeda8f353";
	int testSensorID = 8;
	char *testSensorKey = "f98f1ae87437b01cec83bf80074ac1d6";

	int testSensor = DeviceID_872;
	char jsonresult[256];
	ret = Login(sockfd, testGateID, testGateKey);
	ret = Login(sockfd, testSensorID, testSensorKey);
	ret = Report(sockfd, testSensor, testdatajson);
	ret = Ctrl(sockfd, &testSensor, jsonresult);
	printf("%s\n", jsonresult);
	//getchar();

	/*
	char *testdatajson = "{\"type\":123,\"temp\":2,\"wet\":3,\"state\":1,\"id\":12}";
	char payload[256],revjson[256];
	ParseJson(testdatajson, GuessReportSensor(DeviceID_872), payload);
	ParseBinary(payload, GuessReportSensor(DeviceID_872), revjson);
	printf("%s", revjson);
	getchar();
	*/
	return 0;
}
