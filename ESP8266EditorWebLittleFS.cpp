/*
 * ESP8266EditorWebLittleFS.cpp
 *
 *  Created on: 11-04-2021
 *      Author: Gonzalo
 */

#include "ESP8266EditorWebLittleFS.h"

namespace EditorWebLittleFS {
	ESP8266WebServer *ServidorWeb;
	FS *UnidadFS;
	File uploadFile;
	String nombreArchivo = "";

	template<typename Z> void enviarServidor(Z&& x) {
//			Serial.print(x);
		ServidorWeb->client().print(x);
	}
	template<typename F, typename ... Args> void enviarServidor(F&& x, Args&&... args) {
//			Serial.print(x);
		ServidorWeb->client().print(x);
		enviarServidor(args...);
	}
}

void inicializarEditorWeb(ESP8266WebServer &_servidor, FS &_UnidadFS) {
	EditorWebLittleFS::UnidadFS = &_UnidadFS;
	EditorWebLittleFS::ServidorWeb = &_servidor;
	EditorWebLittleFS::ServidorWeb->on("/list", HTTP_GET, EditorWebLittleFS::printDirectory);
	EditorWebLittleFS::ServidorWeb->on("/edit", HTTP_DELETE, EditorWebLittleFS::handleDelete);
	EditorWebLittleFS::ServidorWeb->on("/edit", HTTP_PUT, EditorWebLittleFS::handleCreate);
	EditorWebLittleFS::ServidorWeb->on("/edit", HTTP_POST, []() {
		EditorWebLittleFS::returnOK();
	}, EditorWebLittleFS::handleFileUpload);
}

void EditorWebLittleFS::returnOK() {
	ServidorWeb->send(200, "text/plain", "");
}

void EditorWebLittleFS::returnFail(String msg) {
	ServidorWeb->send(500, "text/plain", msg + "\r\n");
}

void EditorWebLittleFS::handleFileUpload() {
	if (ServidorWeb->uri() != "/edit") {
		return;
	}
	HTTPUpload& upload = ServidorWeb->upload();

	int indice;
	String nombreFinal;
	switch (upload.status) {
		case UPLOAD_FILE_START:
			indice = upload.filename.lastIndexOf(".src");

			if (indice >= 0) {
				nombreArchivo = upload.filename.substring(0, indice);
			}
			else {
				nombreArchivo = upload.filename;
			}

			nombreArchivo = nombreArchivo + ".temp";

			if (UnidadFS->exists((char *) nombreArchivo.c_str())) {
				UnidadFS->remove((char *) nombreArchivo.c_str());
			}
			uploadFile = UnidadFS->open(nombreArchivo.c_str(), "w");
			Serial.print("Upload: START, filename: ");
			Serial.print(upload.filename);
			Serial.print(" ==> ");
			Serial.println(nombreArchivo);
			break;
		case UPLOAD_FILE_WRITE:
			if (uploadFile) {
				uploadFile.write(upload.buf, upload.currentSize);
			}
			Serial.print("Upload: WRITE, Bytes: ");
			Serial.println(upload.currentSize);
			break;
		case UPLOAD_FILE_END:
			Serial.print("Upload: END, Size: ");
			Serial.println(upload.totalSize);
			if (uploadFile) {
				uploadFile.close();
				nombreFinal = nombreArchivo.substring(0, nombreArchivo.lastIndexOf(".temp"));
				Serial.print("Nombre Act: ");
				Serial.print(nombreArchivo.c_str());
				Serial.print(" --> Nombre Nuevo: ");
				Serial.print(nombreFinal.c_str());
				Serial.print(" --> resultado:  ");
				if (UnidadFS->exists((char *) nombreFinal.c_str())) {
					UnidadFS->remove((char *) nombreFinal.c_str());
				}
				Serial.println(UnidadFS->rename(nombreArchivo.c_str(), nombreFinal.c_str()));
			}
			nombreArchivo = "";
			break;
		case UPLOAD_FILE_ABORTED:
			if (uploadFile) {
				uploadFile.close();
			}
			if (UnidadFS->exists((char *) nombreArchivo.c_str())) {
				UnidadFS->remove((char *) nombreArchivo.c_str());
			}
			nombreArchivo = "";
			break;
	}
}

void EditorWebLittleFS::deleteRecursive(String path) {
	File file = UnidadFS->open((char *) path.c_str(),"r");
	if (!file.isDirectory()) {
		file.close();
		UnidadFS->remove((char *) path.c_str());
		return;
	}

	file.rewindDirectory();
	while (true) {
		File entry = file.openNextFile();
		if (!entry) {
			break;
		}
		String entryPath = path + "/" + entry.name();
		if (entry.isDirectory()) {
			entry.close();
			deleteRecursive(entryPath);
		}
		else {
			entry.close();
			UnidadFS->remove((char *) entryPath.c_str());
		}
		yield();
	}

	UnidadFS->rmdir((char *) path.c_str());
	file.close();
}

void EditorWebLittleFS::handleDelete() {
	if (ServidorWeb->args() == 0) {
		return returnFail("BAD ARGS");
	}
	String path = ServidorWeb->arg(0);
	if (path == "/" || !UnidadFS->exists((char *) path.c_str())) {
		returnFail("BAD PATH");
		return;
	}
	deleteRecursive(path);
	returnOK();
}

void EditorWebLittleFS::handleCreate() {
	if (ServidorWeb->args() == 0) {
		return returnFail("BAD ARGS");
	}
	String path = ServidorWeb->arg(0);
	if (path == "/" || UnidadFS->exists((char *) path.c_str())) {
		returnFail("BAD PATH");
		return;
	}

	if (path.indexOf('.') > 0) {
		File file = UnidadFS->open((char *) path.c_str(), "w");
		if (file) {
			file.write((const char *) 0);
			file.close();
		}
	}
	else {
		UnidadFS->mkdir((char *) path.c_str());
	}
	returnOK();
}

void EditorWebLittleFS::printDirectory() {
	if (!ServidorWeb->hasArg("dir")) {
		return returnFail("BAD ARGS");
	}
	String path = ServidorWeb->arg("dir");
	if (path != "/" && !UnidadFS->exists((char *) path.c_str())) {
		return returnFail("BAD PATH");
	}
	File dir = UnidadFS->open((char *) path.c_str(),"r");

	if (!dir.isDirectory()) {
		dir.close();
		return returnFail("NOT DIR");
	}
	dir.rewindDirectory();
//	ServicioPHV.enviarCabecera(200, "text/json");
	enviarServidor("HTTP/1.1 200 OK\r\n");
	enviarServidor("Content-Type: ", "text/json", "\r\n");
	enviarServidor("Accept-Ranges: none\r\n");
	enviarServidor("Connection: close\r\n");
	enviarServidor("\r\n");
//	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
//	server.send(200, "text/json", "");
//	WiFiClient client = Servidor->client();

	FSInfo datos;
	LittleFS.info(datos);
//	printlnmodsep(" | ", "Capacidad", datos.totalBytes, "Utilizado", datos.usedBytes, "TamPagina", datos.pageSize, "MaxPath", datos.maxPathLength);


	enviarServidor("[");
	String output;
	int elemento = 0;
	if(path == "/"){
		output += "{\"type\":\"";
		output += String(datos.totalBytes);
		output += "\",\"name\":\"";
		output +=String(datos.usedBytes);
		output += "\",\"tam\":\"";
		output +=String(100*float(datos.usedBytes)/float(datos.totalBytes),1);
		output += "\"";
		output += "}";
		enviarServidor(output);
		output="";
		elemento=1;
	}

	for (int cnt=elemento; true; ++cnt) {
		File entry = dir.openNextFile();
		if (!entry) {
			break;
		}

		if (cnt > 0) {
			output = ',';
		}

		output += "{\"type\":\"";
		output += (entry.isDirectory()) ? "dir" : "file";
		output += "\",\"name\":\"";
		output +=entry.name();
		output += "\",\"tam\":\"";
		output +=String(entry.size());
		output += "\"";
		output += "}";
		enviarServidor(output);
		output="";
		entry.close();
	}
	enviarServidor("]");
	ServidorWeb->client().stop();
//	server.sendContent(""); // Terminate the HTTP chunked transmission with a 0-length chunk
	dir.close();
}

