/*
 * ESP8266EditorWebLittleFS.h
 *
 *  Created on: 11-04-2021
 *      Author: Gonzalo
 */

#include "Arduino.h"
#include <LittleFS.h>
#include <ESP8266WebServer.h>

#ifndef LIBRARIES_ESP8266EDITORWEBLITTLEFS_ESP8266EDITORWEBLITTLEFS_H_
#define LIBRARIES_ESP8266EDITORWEBLITTLEFS_ESP8266EDITORWEBLITTLEFS_H_z

void inicializarEditorWeb(ESP8266WebServer &_servidor, FS &_UnidadFS);

namespace EditorWebLittleFS {
	void returnOK();
	void returnFail(String msg);
	void handleFileUpload();
	void deleteRecursive(String path);
	void handleDelete();
	void handleCreate();
	void printDirectory();
}


#endif /* LIBRARIES_ESP8266EDITORWEB_ESP8266EDITORWEB_H_ */
