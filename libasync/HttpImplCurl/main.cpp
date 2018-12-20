#include <getopt.h>
#include "ZQ_common_conf.h"
#include "FileLog.h"

#include "ScriptParser.h"
#include "AllocateServer.h"

static void usage() {
	  printf("\n"
			"-h\tshow this help\n"
			"-c\tscript file path\n"
			"-l\tlog file path\n"
			"-n\tdo not print message\n\n");
}


int main(int argc, char **argv)
{
	  std::string hammerScript, logPath;
	  bool print = true;
	  int opt = 0;
	  
	  while((opt = getopt(argc, argv, "hc:l:n")) != (-1)) {
			switch(opt) {
		case 'h':
			  usage();
			  return (0);
		case 'c':
			  hammerScript = optarg;
			  break;
		case 'l':
			  logPath = optarg;
			  break;
		case 'n':
			  print = false;
			  break;
		default:
			  fprintf(stderr, "unknown option: -?\n");
			  usage();
			  return (-1);
			};
	  }

	  if(hammerScript.empty()) {
			fprintf(stderr, "please specify script file\n");
			usage();	
			return (1);
	  }
	  try{

			HttpImpl::ScriptParser parser(hammerScript);
			std::string hammerLog = (logPath.empty())?(parser._loger.name):(logPath+FNSEPS+parser._loger.name);
			ZQ::common::FileLog logger(hammerLog.c_str(), parser._loger.level, parser._loger.count, parser._loger.size);
			HttpImpl::AllocateServer alloc(logger);
			alloc.init(parser._sessContext);
			alloc.run();
	  }
	  catch(const ZQ::common::ExpatException& e) {
			fprintf(stderr, "%s\n", e.what());
	  }
	  catch(const std::exception& e) {
			fprintf(stderr, "%s\n", e.what());
	  } 
	  catch(...) {
			fprintf(stderr, "unknown error\n");
	  }

	  return 0;
}