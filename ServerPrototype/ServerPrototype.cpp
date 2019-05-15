/////////////////////////////////////////////////////////////////////////
// ServerPrototype.cpp - Console App that processes incoming messages  //
// ver 1.0                                                             //
// Jim Fawcett, CSE687 - Object Oriented Design, Spring 2018           //
/////////////////////////////////////////////////////////////////////////

#include "ServerPrototype.h"
#include "../FileSystem-Windows/FileSystemDemo/FileSystem.h"
#include <chrono>
#include<future>
#include <sstream>

namespace MsgPassComm = MsgPassingCommunication;

using namespace Repository;
using namespace FileSystem;
using Msg = MsgPassingCommunication::Message;

//--------------------< to get the list of files >----------------------------
Files Server::getFiles(const Repository::SearchPath& path)
{
  return Directory::getFiles(path);
}

//--------------------< to get the list of convertefiles >----------------------------
Files Repository::Server::getConvertedFiles()
{
	return Files();
}

//--------------------< to get the list of directories >----------------------------
Dirs Server::getDirs(const Repository::SearchPath& path)
{
  return Directory::getDirectories(path);
}

//--------------------< to show the message >----------------------------
template<typename T>
void show(const T& t, const std::string& msg)
{
  std::cout << "\n  " << msg.c_str();
  for (auto item : t)
  {
    std::cout << "\n    " << item.c_str();
  }
}

//--------------------< to reply for self using echo >----------------------------
std::function<Msg(Msg)> echo = [](Msg msg) {
  Msg reply = msg;
  reply.to(msg.from());
  reply.from(msg.to());
  return reply;
};

//--------------------< callable to get list of files>----------------------------
std::function<Msg(Msg)> getFiles = [](Msg msg) {
  Msg reply;
  reply.to(msg.from());
  reply.from(msg.to());
  reply.command("getFiles");
  std::string path = msg.value("path");
  if (path != "")
  {
    std::string searchPath = storageRoot;
    if (path != ".")
      searchPath = searchPath + "\\" + path;
    Files files = Server::getFiles(searchPath);
    size_t count = 0;
    for (auto item : files)
    {
      std::string countStr = Utilities::Converter<size_t>::toString(++count);
      reply.attribute("file" + countStr, item);
    }
  }
  else
  {
    std::cout << "\n  getFiles message did not define a path attribute";
  }
  return reply;
};

//--------------------< to get the converted files using publish code from executive >----------------------------
void publishfiles(std::promise<std::vector<std::string>> result,std::string path, std::string pattern ,std:: string regex ,std::string option) {
	std::vector<std::string> v1;
	Executive obj;
	obj.setCommandLineArgumentParameter(path, option, pattern, regex);
	result.set_value (obj.convertedFilesList());
}
//--------------------< to get the list of convertedfiles >----------------------------
std::function<Msg(Msg)> converFiles = [](Msg msg) {
	Msg reply;
	std::cout << "\n  Requirment: 6 Converting the source code on the server side";
	std::cout << "\n ==============================================================";
	std::promise< std::vector<std::string>> pro;
	reply.to(msg.from());
	reply.from(msg.to());
	std::cout << std::endl;
	auto fut = pro.get_future();
	std::string searchPath = storageRoot + "\\" + msg.value("path");
	std::async(&publishfiles,std::move(pro),msg.value("path"), msg.value("pattern"), msg.value("regex"), msg.value("option"));
	//t1.detach();
	std::vector<std::string> listOfFiles = fut.get();
	reply.command("convertFiles");
	size_t count = 0;
	for (auto item : listOfFiles) {
		std::string countStr = Utilities::Converter<size_t>::toString(++count);
		reply.attribute("convertFiles" + countStr, item);
	}
	return reply;
};
//--------------------< callable to get a file >----------------------------
std::function<Msg(Msg)> file = [](Msg msg) {
	Msg reply;
	reply.to(msg.from());
	reply.from(msg.to());
	reply.command(msg.attributes()["command"]);
	reply.file(msg.attributes()["name"]);
	return reply;
};

//--------------------< callable to get the list of directories >----------------------------
std::function<Msg(Msg)> getDirs = [](Msg msg) {
  Msg reply;
  reply.to(msg.from());
  reply.from(msg.to());
  reply.command("getDirs");
  std::string path = msg.value("path");
  if (path != "")
  {
    std::string searchPath = storageRoot;
    if (path != ".")
      searchPath = searchPath + "\\" + path;
    Files dirs = Server::getDirs(searchPath);
    size_t count = 0;
    for (auto item : dirs)
    {
      if (item != ".." && item != ".")
      {
        std::string countStr = Utilities::Converter<size_t>::toString(++count);
        reply.attribute("dir" + countStr, item);
      }
    }
  }
  else
  {
    std::cout << "\n  getDirs message did not define a path attribute";
  }
  return reply;
};

//--------------------< main function  >----------------------------
int main(int argc, char *argv[])
{
	std::istringstream iss(argv[1]);
	size_t size;
	iss >> size;
	MsgPassingCommunication::EndPoint endPointDummy("localhost",0);
	if (argc > 1) {
		endPointDummy.port = size;
	}
	if (endPointDummy.port == 0)
		endPointDummy.port = 8080;
	
	serverEndPoint.port = endPointDummy.port;
  std::cout << "\n  Requirment: 5 Server Prototype";
  std::cout << "\n =================================";
  std::cout << "\n";

 Server server(serverEndPoint, "ServerPrototype");
 server.start();

  std::cout << "\n  testing server for getFiles and getDirs methods";
  std::cout << "\n ---------------------------------------------------";
  Files files = server.getFiles();
  show(files, "Files:");
  Dirs dirs = server.getDirs();
  show(dirs, "Dirs:");
  std::cout << "\n";

  std::cout << "\n  testing server for converting files";
  std::cout << "\n ---------------------------------------";
  server.addMsgProc("echo", echo);
  server.addMsgProc("getFiles", getFiles);
  server.addMsgProc("getDirs", getDirs);
  server.addMsgProc("serverQuit", echo);
  server.addMsgProc("getfile", file);
  server.addMsgProc("convertFiles", converFiles);
  server.processMessages();
  
  Msg msg(serverEndPoint, serverEndPoint);  // send to self
  msg.name("msgToSelf");
  msg.command("echo");
  msg.attribute("verbose", "show me");
  server.postMessage(msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  msg.command("getFiles");
  msg.remove("verbose");
  msg.attributes()["path"] = storageRoot;
  server.postMessage(msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  msg.command("getDirs");
  msg.attributes()["path"] = storageRoot;
  server.postMessage(msg);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "\n  press enter to exit";
  std::cin.get();
  std::cout << "\n";

  std::cout << "\n Server is down ";
  std::cout << "\n ==================";
  msg.command("serverQuit");
  server.postMessage(msg);
  server.stop();
  return 0;
}

