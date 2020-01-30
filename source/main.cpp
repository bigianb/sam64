
#include "ram.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include "boost/program_options.hpp"

#include <SDL.h>

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

#include "rom_loader.h"
#include "direct_address_bus.h"
#include "debug_info.h"

struct CLOptions
{
	CLOptions() : errorReported(false), allDone(false) {}

	bool errorReported;
	bool allDone;
	bool dump;
	std::string rombase;
	std::string configbase;
};

static CLOptions parseCommandLineOptions(int argc, char *argv[])
{
	CLOptions retval;

	std::string romBase("./roms");
	std::string configBase("./config");

	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
	("help,h", "Print help messages")
	("dump,d", "Dump disassembly")
	("rombase", po::value<string>()->default_value(romBase), "Rom Base Directory")
	("configbase", po::value<string>()->default_value(configBase), "Config Base Directory");
	po::variables_map vm;

	try
	{
		// throws on error
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

		if (vm.count("help"))
		{
			std::cout << desc << std::endl;
			retval.allDone = true;
			return retval;
		}
		retval.rombase = vm["rombase"].as<std::string>();
		retval.configbase = vm["configbase"].as<std::string>();
		retval.dump = vm.count("dump");
		po::notify(vm);
	}
	catch (boost::program_options::required_option &e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl
				  << std::endl;
		std::cout << desc << std::endl;
		retval.errorReported = true;
		return retval;
	}
	catch (boost::program_options::error &e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl
				  << std::endl;
		std::cout << desc << std::endl;
		retval.errorReported = true;
		return retval;
	}

	return retval;
}

void dump(std::ostream& os, int fromPC, int toPC, DebugInfo& debugInfo, m6502& cpu, AddressBus& bus)
{
	auto irq_vec = bus.readByte(0x3FFF) * 256 + bus.readByte(0x3FFE);

	os << "Initial PC: 0x" << std::setw(4) << std::setfill('0') << std::hex << fromPC << std::endl;
	os << "IRQ vector: 0x" << std::setw(4) << std::setfill('0') << std::hex << irq_vec << std::endl;

	int pc = fromPC;
	while (pc < toPC)
	{
		std::ostringstream stringStream;
		int pc_inc = 1;
		if (debugInfo.getType(pc) == DebugInfo::RangeType::eCODE) {
			auto funcname = debugInfo.getFunctionName(pc);
			if (funcname != "") {
				stringStream << std::endl << funcname << std::endl;
			}
			stringStream << "    " << std::setw(4) << std::setfill('0') << std::hex << pc << ": ";
			auto desc = cpu.disassemble(pc, debugInfo);
			stringStream << desc.line;
			pc_inc = desc.numBytes;
		}
		else {
			stringStream << "    " << std::setw(4) << std::setfill('0') << std::hex << pc << ": ";
			uint8_t val = bus.readByte(pc);
			stringStream << "0x" << std::setw(2) << std::setfill('0') << std::hex << (int)val;
			if (val >= 0x20 && val < 0x80) {
				stringStream << "  " << val;
			}
		}
		auto comment = debugInfo.getComment(pc);
		if (comment != "") {
			auto pos = stringStream.str().length();
			while (pos < 45) {
				stringStream << " ";
				++pos;
			}
			stringStream << "; " << comment;
		}
		os << stringStream.str() << std::endl;
		pc += pc_inc;
	}
}

static const int MILLIS_PER_FRAME = 1000 / 59;

int main(int argc, char *argv[])
{
	Ram ram(64 * 1024);

	CLOptions options = parseCommandLineOptions(argc, argv);
	if (options.allDone)
	{
		return 0;
	}
	if (options.errorReported)
	{
		return -1;
	}

	// load cart data to 0x10000000
	RomLoader romLoader(options.rombase + "/sidetrac");
	romLoader.load("stl8a-1", 0x2800, 0x0800, ram);

    
	DirectAddressBus bus(ram);

	m6502 cpu(bus);

	DebugInfo debugInfo;
	debugInfo.read(options.configbase + "/sidetrac_symbols.json");

	// dip switches. Set before the write handler is installed.
	bus.writeByte(0x5100, 0);

	bus.addWriteHandler(0x5100, [](std::uint32_t addr, std::uint8_t val) {/* std::cout << "wrote " << (int)val << " to 5100" << endl; */ });

	// 0x3f00 is mirrored to 0xFF00
	bus.setMirror(0xFF00, 0xFFFF, 0x3F00);
	cpu.reset();

	auto pc = cpu.regPC;
	if (options.dump) {
		dump(std::cout, pc, 0x3aaa, debugInfo, cpu, bus);
	}
	return runCpu(debugInfo, cpu, bus, graphicsRam);
}

