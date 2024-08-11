/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include <LHVMFile.h>
#include <cxxopts.hpp>

using namespace openblack::LHVM;

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
		RuntimeInfo
	};
	Mode mode {Mode::Header};
	struct Read
	{
		std::filesystem::path filename;
	} read;
};

int PrintInfo(const LHVMFile& file)
{
	std::printf("LHVM Version: %u\n", static_cast<uint32_t>(file.GetHeader().version));
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
	const auto& header = file.GetHeader();
	std::printf("Magic: %.*s\n", 4, header.magic.data());
	std::printf("Version: %u\n", static_cast<uint32_t>(header.version));
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
	std::string s = k_ScriptTypeNames.at(script.GetType()) + " " + script.GetName();
	if (script.GetParameterCount() > 0)
	{
		const auto& vars = script.GetVariables();
		s += vars.at(0);
		for (unsigned int i = 1; i < script.GetParameterCount(); i++)
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
		for (unsigned int i = script.GetInstructionAddress(); i < instructions.size(); i++)
		{
			auto const& instruction = instructions.at(i);
			if (instruction.code == Opcode::JUMP || instruction.code == Opcode::WAIT || instruction.code == Opcode::EXCEPT)
			{
				std::string name = script.GetName();
				if (instruction.code == Opcode::EXCEPT)
				{
					name += "_exception_handler_";
				}
				else if (instruction.mode == VMMode::BACKWARD)
				{
					name += "_loop_";
				}
				else
				{
					name += "_skip_";
				}
				name += std::to_string(labelCount);
				labels.emplace(instruction.uintVal, name);
				labelCount++;
			}
			else if (instruction.code == Opcode::END)
			{
				break;
			}
		}
	}
	return labels;
}

int PrintCode(const LHVMFile& file)
{
	const auto& scripts = file.GetScripts();
	const auto& instructions = file.GetInstructions();
	const auto& labels = GetLabels(file);
	std::printf("Code:\n");
	for (const auto& script : scripts)
	{
		std::printf("begin %s\n", GetSignature(script).c_str());
		// Local vars
		const auto& vars = script.GetVariables();
		for (unsigned int i = script.GetParameterCount(); i < vars.size(); i++)
		{
			std::printf("\tLocal %s\n", vars.at(i).c_str());
		}
		// Code
		std::string opcode;
		std::string type;
		std::string arg;
		for (unsigned int i = script.GetInstructionAddress(); i < instructions.size(); i++)
		{
			if (labels.contains(i))
			{
				std::printf("%s:\n", labels.at(i).c_str());
			}
			auto const& instruction = instructions[i];
			opcode = k_OpcodeNames.at(static_cast<int>(instruction.code));
			switch (instruction.code)
			{
			case Opcode::PUSH:
			case Opcode::POP:
				if (instruction.mode == VMMode::REFERENCE)
				{
					if (instruction.uintVal > script.GetVariablesOffset())
					{
						arg = "local " + script.GetVariables().at(instruction.uintVal - script.GetVariablesOffset() - 1);
					}
					else
					{
						arg = "global " + file.GetVariablesNames().at(instruction.uintVal - 1);
					}
				}
				else
				{
					switch (instruction.type)
					{
					case DataType::FLOAT:
					case DataType::VECTOR:
						arg = std::to_string(instruction.floatVal);
						break;
					case DataType::OBJECT:
						arg = std::to_string(instruction.uintVal);
						break;
					default:
						arg = std::to_string(instruction.intVal);
						break;
					}
				}
				type = k_DataTypeChars.at(static_cast<int>(instruction.type));
				std::printf("\t%s%s %s\n", opcode.c_str(), type.c_str(), arg.c_str());
				break;
			case Opcode::ADD:
			case Opcode::MINUS:
			case Opcode::CAST:
				type = k_DataTypeChars.at(static_cast<int>(instruction.type));
				std::printf("\t%s%s\n", opcode.c_str(), type.c_str());
				break;
			case Opcode::JUMP:
			case Opcode::WAIT:
			case Opcode::EXCEPT:
				arg = labels.at(instruction.uintVal);
				std::printf("\t%s %s\n", opcode.c_str(), arg.c_str());
				break;
			case Opcode::CALL:
				std::printf("\tCALL %u\n", instruction.uintVal);
				break;
			case Opcode::RUN:
				type = instruction.mode == VMMode::ASYNC ? "async " : "";
				arg = file.GetScripts().at(instruction.intVal - 1).GetName();
				std::printf("\tRUN %s%s\n", type.c_str(), arg.c_str());
				break;
			case Opcode::ENDEXCEPT:
				if (instruction.mode == VMMode::ENDEXCEPT)
				{
					std::printf("\tENDEXCEPT\n");
				}
				else // Mode::YIELD
				{
					std::printf("\tYIELD\n");
				}
				break;
			case Opcode::SWAP:
				if (instruction.type == DataType::INT)
				{
					std::printf("\tSWAP\n");
				}
				else
				{
					if (instruction.mode == VMMode::COPYFROM)
					{
						std::printf("\tCOPY from %i\n", instruction.intVal);
					}
					else // Mode::COPYTO
					{
						std::printf("\tCOPY to %i\n", instruction.intVal);
					}
				}
				break;
			default:
				std::printf("\t%s\n", opcode.c_str());
			}
			if (instruction.code == Opcode::END)
			{
				break;
			}
		}
		std::printf("\n");
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
		const auto& script = scripts[scriptId - 1];
		std::printf("%s\n", script.GetName().c_str());
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
		std::printf("%s = %u\n", GetSignature(script).c_str(), script.GetInstructionAddress());
	}
	std::printf("\n");
	return EXIT_SUCCESS;
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
	case DataType::INT:
		return std::to_string(data.intVal);
	case DataType::FLOAT:
	case DataType::VECTOR:
		return std::to_string(data.floatVal) + "f";
	case DataType::BOOLEAN:
		return data.intVal != 0 ? "true" : "false";
	case DataType::OBJECT:
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
	const auto& stack = file.GetStack();
	PrintStack(stack);
	return EXIT_SUCCESS;
}

int PrintVarValues(const LHVMFile& file)
{
	const auto& vars = file.GetVariablesValues();
	std::printf("Global variables values:\n");
	for (unsigned int i = 0; i < vars.size(); i++)
	{
		const auto& var = vars[i];
		std::printf("%u, %s = %s\n", i, var.name.c_str(), DataToString(var.value, var.type).c_str());
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintTasks(const LHVMFile& file)
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
		std::printf("Instruction address: %u\n", task.instructionAddress);
		std::printf("Prev instruction address: %u\n", task.pevInstructionAddress);
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
			std::printf("%u\n", ip);
		}
		std::printf("\n");
		std::printf("Local variables offset: %u\n", task.variablesOffset);
		std::printf("Variables:\n");
		const auto& vars = task.localVars;
		for (unsigned int i = 0; i < vars.size(); i++)
		{
			const auto& var = vars[i];
			const int id = task.variablesOffset + 1 + i;
			std::printf("%i, %s = %s\n", id, var.name.c_str(), DataToString(var.value, var.type).c_str());
		}
		std::printf("\n");
		PrintStack(task.stack);
		std::printf("----------------------------------------\n");
		std::printf("\n");
	}
	std::printf("\n");
	return EXIT_SUCCESS;
}

int PrintRuntimeInfo(const LHVMFile& file)
{
	std::printf("Ticks count: %u\n", file.GetTicks());
	std::printf("Current line number: %u\n", file.GetCurrentLineNumber());
	std::printf("Highest task ID: %u\n", file.GetHighestTaskId());
	std::printf("Highest script ID: %u\n", file.GetHighestScriptId());
	std::printf("Executed instructions: %u\n", file.GetExecutedInstructions());
	std::printf("\n");
	return EXIT_SUCCESS;
}

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("lhvmtool", "Inspect and extract files from LionHead Virtual Machine files.");

	try
	{
		options.add_options()("h,help", "Display this help message.")("subcommand", "Subcommand.",
		                                                              cxxopts::value<std::string>());
		options.positional_help("[read] [OPTION...]");
		options.add_options("read")("I,info", "Print info.", cxxopts::value<std::string>())("A,all", "Print all relevant data.",
		                                                                                    cxxopts::value<std::string>())(
		    "H,header", "Print header contents.", cxxopts::value<std::string>())("G,globals", "Print global var names.",
		                                                                         cxxopts::value<std::string>())(
		    "C,code", "Print asm code.", cxxopts::value<std::string>())("a,autostart", "Print autostart scripts.",
		                                                                cxxopts::value<std::string>())(
		    "S,scripts", "Print scripts.", cxxopts::value<std::string>())("D,data", "Print data.",
		                                                                  cxxopts::value<std::string>())(
		    "s,stack", "Print global stack.", cxxopts::value<std::string>())("V,values", "Print global var values.",
		                                                                     cxxopts::value<std::string>())(
		    "T,tasks", "Print active tasks.", cxxopts::value<std::string>())("R,rtinfo", "Print runtime info.",
		                                                                     cxxopts::value<std::string>());

		options.parse_positional({"subcommand"});
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		returnCode = EXIT_FAILURE;
		return false;
	}

	try
	{
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
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << '\n';
	}

	std::cerr << options.help() << '\n';
	returnCode = EXIT_FAILURE;
	return false;
}

int main(int argc, char* argv[]) noexcept
{
	Arguments args;
	int returnCode = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, returnCode))
	{
		return returnCode;
	}

	LHVMFile file;
	try
	{
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
			returnCode |= PrintCode(file);
			break;
		case Arguments::Mode::Autostart:
			returnCode |= PrintAutostart(file);
			break;
		case Arguments::Mode::Scripts:
			returnCode |= PrintScripts(file);
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
	}
	catch (std::exception& err)
	{
		std::cerr << err.what() << '\n';
		returnCode |= EXIT_FAILURE;
	}

	return returnCode;
}
