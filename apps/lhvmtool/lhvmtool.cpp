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
		RuntimeInfo
	};
	Mode mode {Mode::Header};
	struct Read
	{
		std::filesystem::path filename;
		std::string objName;
	} read;
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
				else if (instruction.uintVal < i) // Backward jumps are used for 'while' loops
				{
					name += "_loop_";
				}
				else // Forward jumps are used for if-then-else
				{
					name += "_skip_";
				}
				name += std::to_string(labelCount);
				labels.emplace(instruction.uintVal, name);
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
						const auto varId = static_cast<size_t>(instruction.uintVal);
						if (instruction.uintVal > script.variablesOffset)
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
							arg = std::to_string(instruction.floatVal);
							break;
						case DataType::Object:
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
				case Opcode::Add:
				case Opcode::Sub:
				case Opcode::Cast:
					type = k_DataTypeChars.at(static_cast<int>(instruction.type));
					std::printf("\t%s%s\n", opcode.c_str(), type.c_str());
					break;
				case Opcode::Jmp:
				case Opcode::Wait:
				case Opcode::Except:
					arg = labels.at(instruction.uintVal);
					std::printf("\t%s %s\n", opcode.c_str(), arg.c_str());
					break;
				case Opcode::Sys:
					std::printf("\tCALL %u\n", instruction.uintVal);
					break;
				case Opcode::Run:
					type = instruction.mode == VMMode::Async ? "async " : "";
					arg = file.GetScripts().at(static_cast<size_t>(instruction.intVal) - 1).name;
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
							std::printf("\tCOPY from %i\n", instruction.intVal);
						}
						else // Mode::CopyTo
						{
							std::printf("\tCOPY to %i\n", instruction.intVal);
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
		std::printf("%s = %u\n", GetSignature(script).c_str(), script.instructionAddress);
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
			std::printf("Address:     %u\n", script.instructionAddress);
			std::printf("Vars offset: %u\n", script.variablesOffset);
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

bool parseOptions(int argc, char** argv, Arguments& args, int& returnCode) noexcept
{
	cxxopts::Options options("lhvmtool", "Inspect and extract files from LionHead Virtual Machine files.");

	options.add_options()                                            //
		("h,help", "Display this help message.")                     //
		("subcommand", "Subcommand.", cxxopts::value<std::string>()) //
		;
	options.positional_help("[read] [OPTION...]");
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

	options.parse_positional({"subcommand"});
	
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
	Arguments args;
	int returnCode = EXIT_SUCCESS;
	if (!parseOptions(argc, argv, args, returnCode))
	{
		return returnCode;
	}

	LHVMFile file;
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
