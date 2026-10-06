/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

#include <ChlConstants.h>
#include <LHVMDecompiler.h>
#include <LHVMFile.h>
#include <cxxopts.hpp>
#include <fmt/format.h>

#include "lhvmtool_compile.h"

using namespace openblack::lhvm;

struct Arguments
{
	enum class Mode : uint8_t
	{
		Info,
		All,
		Header,
		Vars,
		Code,
		Autostart,
		Scripts,
		Data,
		Stack,
		VarValues,
		Tasks,
		RuntimeInfo,
		Decompile
	};
	Mode mode {Mode::Header};
	struct Read
	{
		std::filesystem::path filename;
		std::string objName;
	} read;
	struct Decompile
	{
		std::filesystem::path output;
		std::filesystem::path headers;
		bool stats {false};
		bool addresses {false};
		bool diagnostics {false};
		bool sourceLines {false};
	} decompile;
};

int PrintInfo(const LHVMFile& file)
{
	std::printf("LHVM Version: %u\n", static_cast<uint32_t>(file.GetVersion()));
	std::printf("Global vars count: %zu\n", file.GetVariablesNames().size());
	std::printf("Scripts count: %zu\n", file.GetScripts().size());
	std::printf("Instructions count: %zu\n", file.GetInstructions().size());
	std::printf("Autostart count: %zu\n", file.GetAutostart().size());
	std::printf("Data size: %zu\n", file.GetData().size());
	std::printf("Autostart count: %zu\n", file.GetAutostart().size());
	std::printf("\n");
	if (file.HasStatus())
	{
		std::printf("--- Status data ---\n");
		std::printf("Tasks count: %zu\n", file.GetTasks().size());
		std::printf("Ticks count: %u\n", file.GetTicks());
		std::printf("Current line number: %u\n", file.GetCurrentLineNumber());
		std::printf("Highest task ID: %u\n", file.GetHighestTaskId());
		std::printf("Highest script ID: %u\n", file.GetHighestScriptId());
		std::printf("Executed instructions: %u\n", file.GetExecutedInstructions());
	}
	else
	{
		std::printf("--- No status data ---\n");
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintHeader(const LHVMFile& file)
{
	std::printf("Version: %u\n", static_cast<uint32_t>(file.GetVersion()));
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintVarNames(const LHVMFile& file)
{
	const auto& names = file.GetVariablesNames();
	int id = 1; // id 0 is reserved for "Null variable", which is added at runtime
	std::printf("Global variables:\n");
	for (const auto& name : names)
	{
		std::printf("%i: %s\n", id++, name.c_str());
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

std::string GetSignature(const VMScript& script)
{
	std::string s = k_ScriptTypeNames.at(script.type) + " " + script.name + "(";
	if (script.parameterCount > 0)
	{
		const auto& vars = script.variables;
		s += vars.at(0);
		for (unsigned int i = 1; i < script.parameterCount; i++)
		{
			s += ", " + vars.at(i);
		}
	}
	s += ")";
	return s;
}

std::map<uint32_t, std::string> GetLabels(const LHVMFile& file)
{
	std::map<uint32_t, std::string> labels;
	const auto& scripts = file.GetScripts();
	const auto& instructions = file.GetInstructions();
	for (const auto& script : scripts)
	{
		int labelCount = 0;
		for (unsigned int i = script.instructionAddress; i < instructions.size(); i++)
		{
			auto const& instruction = instructions.at(i);
			if (instruction.code == Opcode::Jmp || instruction.code == Opcode::Wait || instruction.code == Opcode::Except)
			{
				std::string name = script.name;
				if (instruction.code == Opcode::Except)
				{
					name += "_exception_handler_";
				}
				else if (instruction.data.uintVal < i) // Backward jumps are used for 'while' loops
				{
					name += "_loop_";
				}
				else // Forward jumps are used for if-then-else
				{
					name += "_skip_";
				}
				name += std::to_string(labelCount);
				labels.emplace(instruction.data.uintVal, name);
				labelCount++;
			}
			else if (instruction.code == Opcode::End)
			{
				break;
			}
		}
	}
	return labels;
}

int PrintCode(const LHVMFile& file, const std::string& name)
{
	const auto& scripts = file.GetScripts();
	const auto& instructions = file.GetInstructions();
	const auto& labels = GetLabels(file);
	std::printf("Code:\n");
	for (const auto& script : scripts)
	{
		if (name.empty() || name == script.name)
		{
			std::printf("begin %s\n", GetSignature(script).c_str());
			// Local vars
			const auto& vars = script.variables;
			for (unsigned int i = script.parameterCount; i < vars.size(); i++)
			{
				std::printf("\tLocal %s\n", vars.at(i).c_str());
			}
			// Code
			std::string opcode;
			std::string type;
			std::string arg;
			for (unsigned int i = script.instructionAddress; i < instructions.size(); i++)
			{
				if (labels.contains(i))
				{
					std::printf("%s:\n", labels.at(i).c_str());
				}
				auto const& instruction = instructions[i];
				opcode = k_OpcodeNames.at(static_cast<int>(instruction.code));
				switch (instruction.code)
				{
				case Opcode::Push:
				case Opcode::Pop:
					if (instruction.mode == VMMode::Reference)
					{
						const auto varId = static_cast<size_t>(instruction.data.uintVal);
						if (instruction.data.uintVal > script.variablesOffset)
						{
							arg = "local " + script.variables.at(varId - script.variablesOffset - 1);
						}
						else
						{
							arg = "global " + file.GetVariablesNames().at(varId - 1);
						}
					}
					else
					{
						switch (instruction.type)
						{
						case DataType::Float:
						case DataType::Vector:
							arg = std::to_string(instruction.data.floatVal);
							break;
						case DataType::Object:
							arg = std::to_string(instruction.data.uintVal);
							break;
						default:
							arg = std::to_string(instruction.data.intVal);
							break;
						}
					}
					type = k_DataTypeChars.at(static_cast<int>(instruction.type));
					std::printf("\t%s%s %s\n", opcode.c_str(), type.c_str(), arg.c_str());
					break;
				case Opcode::Add:
				case Opcode::Sub:
				case Opcode::Cast:
					type = k_DataTypeChars.at(static_cast<int>(instruction.type));
					std::printf("\t%s%s\n", opcode.c_str(), type.c_str());
					break;
				case Opcode::Jmp:
				case Opcode::Wait:
				case Opcode::Except:
					arg = labels.at(instruction.data.uintVal);
					std::printf("\t%s %s\n", opcode.c_str(), arg.c_str());
					break;
				case Opcode::Sys:
					std::printf("\tCALL %u\n", instruction.data.uintVal);
					break;
				case Opcode::Run:
					type = instruction.mode == VMMode::Async ? "async " : "";
					arg = file.GetScripts().at(static_cast<size_t>(instruction.data.intVal) - 1).name;
					std::printf("\tRUN %s%s\n", type.c_str(), arg.c_str());
					break;
				case Opcode::EndExcept:
					if (instruction.mode == VMMode::EndExcept)
					{
						std::printf("\tENDEXCEPT\n");
					}
					else // Mode::Yield
					{
						std::printf("\tYIELD\n");
					}
					break;
				case Opcode::Swap:
					if (instruction.type == DataType::Int)
					{
						std::printf("\tSWAP\n");
					}
					else
					{
						if (instruction.mode == VMMode::CopyFrom)
						{
							std::printf("\tCOPY from %i\n", instruction.data.intVal);
						}
						else // Mode::CopyTo
						{
							std::printf("\tCOPY to %i\n", instruction.data.intVal);
						}
					}
					break;
				default:
					std::printf("\t%s\n", opcode.c_str());
				}
				if (instruction.code == Opcode::End)
				{
					break;
				}
			}
			std::printf("\n");
			if (!name.empty())
			{
				return EXIT_SUCCESS;
			}
		}
	}
	if (!name.empty())
	{
		std::printf("Script not found\n");
		return EXIT_FAILURE;
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintAutostart(const LHVMFile& file)
{
	const auto& scripts = file.GetScripts();
	const auto& autostart = file.GetAutostart();
	std::printf("Autostart scripts:\n");
	for (const auto& scriptId : autostart)
	{
		const auto& script = scripts.at(static_cast<size_t>(scriptId) - 1);
		std::printf("%s\n", script.name.c_str());
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintScripts(const LHVMFile& file)
{
	const auto& scripts = file.GetScripts();
	std::printf("Scripts:\n");
	for (const auto& script : scripts)
	{
		std::printf("%s = 0x%04x\n", GetSignature(script).c_str(), script.instructionAddress);
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintScript(const LHVMFile& file, const std::string& name)
{
	const auto& scripts = file.GetScripts();
	for (const auto& script : scripts)
	{
		if (script.name == name)
		{
			std::printf("%s\n", GetSignature(script).c_str());
			std::printf("Source:      %s\n", script.filename.c_str());
			std::printf("ID:          %u\n", script.scriptId);
			std::printf("Address:     0x%04x\n", script.instructionAddress);
			std::printf("Vars offset: 0x%04x\n", script.variablesOffset);
			std::printf("Local vars:\n");
			for (size_t i = script.parameterCount; i < script.variables.size(); i++)
			{
				std::printf("\tlocal %s\n", script.variables.at(i).c_str());
			}
			return EXIT_SUCCESS;
		}
	}
	std::printf("Script not found\n");
	return EXIT_FAILURE;
}

int PrintData(const LHVMFile& file)
{
	const auto& data = file.GetData();
	std::printf("Data:\n");
	size_t offset = 0;
	while (offset < data.size())
	{
		const char* str = &data[offset];
		std::printf("%zu: %s\n", offset, str);
		offset += strlen(str) + 1;
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

std::string DataToString(VMValue data, DataType type)
{
	switch (type)
	{
	case DataType::Int:
		return std::to_string(data.intVal);
	case DataType::Float:
	case DataType::Vector:
		return std::to_string(data.floatVal) + "f";
	case DataType::Boolean:
		return data.intVal != 0 ? "true" : "false";
	case DataType::Object:
		return std::to_string(data.uintVal) + " (object)";
	default:
		return std::to_string(data.intVal) + " (unk type)";
	}
}

int PrintStack(const VMStack& stack)
{
	std::printf("Stack:\n");
	for (unsigned int i = 0; i < stack.count; i++)
	{
		std::printf("%u: %s\n", i, DataToString(stack.values.at(i), stack.types.at(i)).c_str());
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintGlobalStack(const LHVMFile& file)
{
	if (file.HasStatus())
	{
		const auto& stack = file.GetStack();
		PrintStack(stack);
	}
	else
	{
		std::printf("This file hasn't status data\n");
	}
	return EXIT_SUCCESS;
}

int PrintVarValues(const LHVMFile& file)
{
	if (file.HasStatus())
	{
		const auto& vars = file.GetVariablesValues();
		std::printf("Global variables values:\n");
		for (unsigned int i = 0; i < vars.size(); i++)
		{
			const auto& var = vars[i];
			std::printf("%u, %s = %s\n", i, var.name.c_str(), DataToString(var.value, var.type).c_str());
		}
		std::printf("\n");
	}
	else
	{
		std::printf("This file hasn't status data\n");
	}
	return EXIT_SUCCESS;
}

int PrintTasks(const LHVMFile& file)
{
	if (file.HasStatus())
	{
		const auto& tasks = file.GetTasks();
		std::printf("Active tasks:\n");
		for (const auto& task : tasks)
		{
			std::printf("Task number: %u\n", task.id);
			std::printf("Type: %s\n", k_ScriptTypeNames.at(task.type).c_str());
			std::printf("Script ID: %u\n", task.scriptId);
			std::printf("Script name: %s\n", task.name.c_str());
			std::printf("Filename: %s\n", task.filename.c_str());
			std::printf("Instruction address: 0x%04x\n", task.instructionAddress);
			std::printf("Prev instruction address: 0x%04x\n", task.pevInstructionAddress);
			std::printf("Ticks: %u\n", task.ticks);
			std::printf("Sleeping: %s\n", task.sleeping ? "true" : "false");
			std::printf("Waiting task number: %u\n", task.waitingTaskId);
			std::printf("Stop: %s\n", task.stop ? "true" : "false");
			std::printf("Yield: %s\n", task.iield ? "true" : "false");
			std::printf("In exception handler: %s\n", task.inExceptionHandler ? "true" : "false");
			std::printf("Current exception handler index: %d\n", task.currentExceptionHandlerIndex);
			std::printf("Exception handlers instructions pointers:\n");
			for (const auto& ip : task.exceptionHandlerIps)
			{
				std::printf("0x%04x\n", ip);
			}
			std::printf("\n");
			std::printf("Local variables offset: 0x%04x\n", task.variablesOffset);
			std::printf("Variables:\n");
			const auto& vars = task.localVars;
			for (unsigned int i = 0; i < vars.size(); i++)
			{
				const auto& var = vars[i];
				const int id = task.variablesOffset + 1 + i;
				std::printf("0x%04x, %s = %s\n", id, var.name.c_str(), DataToString(var.value, var.type).c_str());
			}
			std::printf("\n");
			PrintStack(task.stack);
			std::printf("----------------------------------------\n");
			std::printf("\n");
		}
		std::printf("\n");
	}
	else
	{
		std::printf("This file hasn't status data\n");
	}
	return EXIT_SUCCESS;
}

int PrintRuntimeInfo(const LHVMFile& file)
{
	if (file.HasStatus())
	{
		std::printf("Ticks count: %u\n", file.GetTicks());
		std::printf("Current line number: %u\n", file.GetCurrentLineNumber());
		std::printf("Highest task ID: %u\n", file.GetHighestTaskId());
		std::printf("Highest script ID: %u\n", file.GetHighestScriptId());
		std::printf("Executed instructions: %u\n", file.GetExecutedInstructions());
		std::printf("\n");
	}
	else
	{
		std::printf("This file hasn't status data\n");
	}
	return EXIT_SUCCESS;
}

/// The script's text, each line prefixed with the first instruction address it stands for when asked
std::string ScriptText(const DecompiledScript& script, bool addresses)
{
	if (!addresses)
	{
		return script.text;
	}
	std::string text;
	size_t line = 0;
	size_t start = 0;
	while (start < script.text.size())
	{
		const auto end = script.text.find('\n', start);
		const auto& ips = script.lines.at(line++);
		text += ips.empty() ? std::string(8, ' ') : fmt::format("{:6}  ", ips.front());
		text += script.text.substr(start, end - start + 1);
		start = end + 1;
	}
	return text;
}

int Decompile(const LHVMFile& file, const Arguments& args)
{
	const auto program = ProgramView::From(file);
	openblack::lhvm::chl::ConstantTable constants;
	DecompileOptions options;
	options.sourceLines = args.decompile.sourceLines;
	if (!args.decompile.headers.empty())
	{
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(args.decompile.headers, ec))
		{
			const auto extension = entry.path().extension();
			if (extension == ".h" || extension == ".txt")
			{
				std::ifstream stream(entry.path(), std::ios::binary);
				const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
				if (extension == ".h")
				{
					constants.LoadHeader(text);
				}
				else
				{
					constants.LoadInfo(text);
				}
			}
		}
		options.constants = &constants;
	}

	// One script on its own, or the whole program as the files it was compiled from
	DecompiledProgram whole;
	if (!args.read.objName.empty())
	{
		const auto& all = file.GetScripts();
		const auto it = std::ranges::find(all, args.read.objName, &VMScript::name);
		if (it == all.end())
		{
			std::fprintf(stderr, "Script not found\n");
			return EXIT_FAILURE;
		}
		whole.scripts.push_back(DecompileScript(program, static_cast<size_t>(std::distance(all.begin(), it)), options));
		whole.files.push_back({.name = it->filename, .header = {}, .scripts = {0}});
	}
	else
	{
		whole = DecompileAll(program, options);
	}
	const auto& scripts = whole.scripts;
	const auto fileText = [&](const DecompiledFile& decompiledFile) {
		if (whole.sourceLines)
		{
			return whole.FileText(decompiledFile);
		}
		std::string text = decompiledFile.header;
		for (const auto index : decompiledFile.scripts)
		{
			text += (text.empty() ? "" : "\n") + ScriptText(scripts[index], args.decompile.addresses);
		}
		return text;
	};

	if (args.decompile.output.empty())
	{
		std::string text;
		for (const auto& decompiledFile : whole.files)
		{
			text += (text.empty() ? "" : "\n") + fileText(decompiledFile);
		}
		std::fwrite(text.data(), 1, text.size(), stdout);
	}
	else
	{
		// One file per source file the scripts were compiled from
		std::error_code ec;
		std::filesystem::create_directories(args.decompile.output, ec);
		std::map<std::string, std::string> files;
		for (const auto& decompiledFile : whole.files)
		{
			auto name = std::filesystem::path(decompiledFile.name).filename().string();
			if (name.empty())
			{
				name = "Unnamed.txt";
			}
			auto& text = files[name];
			text += (text.empty() ? "" : "\n") + fileText(decompiledFile);
		}
		for (const auto& [name, text] : files)
		{
			std::ofstream stream(args.decompile.output / name, std::ios::binary);
			stream << text;
		}
		std::fprintf(stderr, "Wrote %zu files to %s\n", files.size(), args.decompile.output.string().c_str());
	}

	if (args.decompile.diagnostics)
	{
		static constexpr std::array<const char*, 3> k_Severity = {"info", "warning", "error"};
		for (const auto& script : scripts)
		{
			for (const auto& diagnostic : script.diagnostics)
			{
				std::fprintf(stderr, "%s:%u: %s: %s\n", script.name.c_str(), diagnostic.ip,
				             k_Severity.at(static_cast<size_t>(diagnostic.severity)), diagnostic.message.c_str());
			}
		}
	}
	if (args.decompile.stats)
	{
		DecompileStats stats;
		for (const auto& script : scripts)
		{
			++stats.scripts;
			stats.instructions += script.endIp - script.firstIp;
			stats.unaccountedInstructions += script.unaccountedCount;
			stats.unsupportedOpcodes += script.unsupportedCount;
			stats.nativeCalls += script.nativeCallCount;
			stats.gotos += script.gotoCount;
			stats.clean += script.IsClean() ? 1 : 0;
			stats.withGoto += script.gotoCount > 0 ? 1 : 0;
			stats.withFallback += script.fallbackCount > 0 ? 1 : 0;
		}
		std::fprintf(stderr, "Scripts: %zu\n", stats.scripts);
		std::fprintf(stderr, "Structured cleanly: %zu\n", stats.clean);
		std::fprintf(stderr, "Needing goto: %zu (%zu gotos)\n", stats.withGoto, stats.gotos);
		std::fprintf(stderr, "With other fallbacks: %zu\n", stats.withFallback);
		std::fprintf(stderr, "Instructions: %zu\n", stats.instructions);
		std::fprintf(stderr, "Instructions not shown: %zu\n", stats.unaccountedInstructions);
		std::fprintf(stderr, "Unsupported instructions: %zu\n", stats.unsupportedOpcodes);
		std::fprintf(stderr, "Native calls without a statement form: %zu\n", stats.nativeCalls);
		for (const auto& script : scripts)
		{
			if (!script.IsClean())
			{
				std::fprintf(stderr, "  not clean: %s (goto %u, fallbacks %u, not shown %u)\n", script.name.c_str(),
				             script.gotoCount, script.fallbackCount, script.unaccountedCount);
			}
		}
	}
	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("lhvmtool", "Inspect and extract files from LionHead Virtual Machine files.");

	options.add_options()                                            //
	    ("h,help", "Display this help message.")                     //
	    ("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
	    ("input", "Input file.", cxxopts::value<std::string>())      //
	    ;
	options.positional_help("[read|decompile FILE|compile SOURCES] [OPTION...]");
	options.add_options("read")                                                     //
	    ("I,info", "Print info.", cxxopts::value<std::string>())                    //
	    ("A,all", "Print all relevant data.", cxxopts::value<std::string>())        //
	    ("H,header", "Print header contents.", cxxopts::value<std::string>())       //
	    ("G,globals", "Print global var names.", cxxopts::value<std::string>())     //
	    ("C,code", "Print asm code.", cxxopts::value<std::string>())                //
	    ("a,autostart", "Print autostart scripts.", cxxopts::value<std::string>())  //
	    ("S,scripts", "Print scripts.", cxxopts::value<std::string>())              //
	    ("D,data", "Print data.", cxxopts::value<std::string>())                    //
	    ("s,stack", "Print global stack.", cxxopts::value<std::string>())           //
	    ("V,values", "Print global var values.", cxxopts::value<std::string>())     //
	    ("T,tasks", "Print active tasks.", cxxopts::value<std::string>())           //
	    ("R,rtinfo", "Print runtime info.", cxxopts::value<std::string>())          //
	    ("n,name", "Object name", cxxopts::value<std::string>()->default_value("")) //
	    ;

	options.add_options("decompile")                                                              //
	    ("o,output", "Write to this directory instead of stdout.", cxxopts::value<std::string>()) //
	    ("headers", "Directory of script headers (.h) and info tables (.txt) to name constants.",
	     cxxopts::value<std::string>())                                       //
	    ("stats", "Print how well the scripts decompiled.")                   //
	    ("addresses", "Prefix each line with its first instruction address.") //
	    ("source-lines", "Place statements on their recorded source lines.")("diagnostics",
	                                                                         "Print the decompiler's diagnostics.") //
	    ;

	options.parse_positional({"subcommand", "input"});
	auto result = options.parse(argc, argv);
	if (result["help"].as<bool>())
	{
		std::cout << options.help() << '\n';
		returnCode = EXIT_SUCCESS;
		return false;
	}
	if (result["subcommand"].count() == 0)
	{
		std::cerr << options.help() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}
	if (result["subcommand"].as<std::string>() == "decompile" && result["input"].count() > 0)
	{
		args.mode = Arguments::Mode::Decompile;
		args.read.filename = result["input"].as<std::string>();
		args.read.objName = result["name"].as<std::string>();
		if (result["output"].count() > 0)
		{
			args.decompile.output = result["output"].as<std::string>();
		}
		if (result["headers"].count() > 0)
		{
			args.decompile.headers = result["headers"].as<std::string>();
		}
		args.decompile.stats = result["stats"].as<bool>();
		args.decompile.addresses = result["addresses"].as<bool>();
		args.decompile.diagnostics = result["diagnostics"].as<bool>();
		args.decompile.sourceLines = result["source-lines"].as<bool>();
		return true;
	}
	if (result["subcommand"].as<std::string>() == "read")
	{
		if (result["info"].count() > 0)
		{
			args.mode = Arguments::Mode::Info;
			args.read.filename = result["info"].as<std::string>();
			return true;
		}
		if (result["all"].count() > 0)
		{
			args.mode = Arguments::Mode::All;
			args.read.filename = result["all"].as<std::string>();
			return true;
		}
		if (result["header"].count() > 0)
		{
			args.mode = Arguments::Mode::Header;
			args.read.filename = result["header"].as<std::string>();
			return true;
		}
		if (result["globals"].count() > 0)
		{
			args.mode = Arguments::Mode::Vars;
			args.read.filename = result["globals"].as<std::string>();
			return true;
		}
		if (result["code"].count() > 0)
		{
			args.mode = Arguments::Mode::Code;
			args.read.filename = result["code"].as<std::string>();
			args.read.objName = result["name"].as<std::string>();
			return true;
		}
		if (result["autostart"].count() > 0)
		{
			args.mode = Arguments::Mode::Autostart;
			args.read.filename = result["autostart"].as<std::string>();
			return true;
		}
		if (result["scripts"].count() > 0)
		{
			args.mode = Arguments::Mode::Scripts;
			args.read.filename = result["scripts"].as<std::string>();
			args.read.objName = result["name"].as<std::string>();
			return true;
		}
		if (result["data"].count() > 0)
		{
			args.mode = Arguments::Mode::Data;
			args.read.filename = result["data"].as<std::string>();
			return true;
		}
		if (result["stack"].count() > 0)
		{
			args.mode = Arguments::Mode::Stack;
			args.read.filename = result["stack"].as<std::string>();
			return true;
		}
		if (result["values"].count() > 0)
		{
			args.mode = Arguments::Mode::VarValues;
			args.read.filename = result["values"].as<std::string>();
			return true;
		}
		if (result["tasks"].count() > 0)
		{
			args.mode = Arguments::Mode::Tasks;
			args.read.filename = result["tasks"].as<std::string>();
			return true;
		}
		if (result["rtinfo"].count() > 0)
		{
			args.mode = Arguments::Mode::RuntimeInfo;
			args.read.filename = result["rtinfo"].as<std::string>();
			return true;
		}
	}
	std::cerr << options.help() << '\n';
	returnCode = EXIT_FAILURE;
	return false;
}

int main(int argc, char* argv[]) noexcept
{
	if (argc > 1 && std::string_view(argv[1]) == "compile")
	{
		return RunCompile(argc - 1, argv + 1);
	}

	Arguments args;
	int returnCode = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, returnCode))
	{
		return returnCode;
	}

	LHVMFile file;
	if (args.mode == Arguments::Mode::Decompile)
	{
		file.Open(args.read.filename);
		if (!file.IsLoaded())
		{
			std::fprintf(stderr, "Can't read %s\n", args.read.filename.string().c_str());
			return EXIT_FAILURE;
		}
		return Decompile(file, args);
	}
	std::printf("Filename: %s\n", args.read.filename.string().c_str());

	// Open file
	file.Open(args.read.filename);

	switch (args.mode)
	{
	case Arguments::Mode::Info:
		returnCode |= PrintInfo(file);
		break;
	case Arguments::Mode::All:
		returnCode |= PrintInfo(file);
		returnCode |= PrintAutostart(file);
		returnCode |= PrintVarNames(file);
		returnCode |= PrintScripts(file);
		returnCode |= PrintData(file);
		if (file.HasStatus())
		{
			returnCode |= PrintRuntimeInfo(file);
			returnCode |= PrintVarValues(file);
			returnCode |= PrintGlobalStack(file);
			returnCode |= PrintTasks(file);
		}
		break;
	case Arguments::Mode::Header:
		returnCode |= PrintHeader(file);
		break;
	case Arguments::Mode::Vars:
		returnCode |= PrintVarNames(file);
		break;
	case Arguments::Mode::Code:
		returnCode |= PrintCode(file, args.read.objName);
		break;
	case Arguments::Mode::Autostart:
		returnCode |= PrintAutostart(file);
		break;
	case Arguments::Mode::Scripts:
		if (args.read.objName.empty())
		{
			returnCode |= PrintScripts(file);
		}
		else
		{
			returnCode |= PrintScript(file, args.read.objName);
		}
		break;
	case Arguments::Mode::Data:
		returnCode |= PrintData(file);
		break;
	case Arguments::Mode::Stack:
		returnCode |= PrintGlobalStack(file);
		break;
	case Arguments::Mode::VarValues:
		returnCode |= PrintVarValues(file);
		break;
	case Arguments::Mode::Tasks:
		returnCode |= PrintTasks(file);
		break;
	case Arguments::Mode::RuntimeInfo:
		returnCode |= PrintRuntimeInfo(file);
		break;

	default:
		returnCode = EXIT_FAILURE;
		break;
	}
	return returnCode;
}
