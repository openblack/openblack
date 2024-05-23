# Style Guidelines for openblack

## Formatting Guidelines

Every pull request you submit will be tested using the configurations specified in our [`.clang-format`](https://github.com/openblack/openblack/blob/master/.clang-format), [`.clang-tidy`](https://github.com/openblack/openblack/blob/master/.clang-tidy), [`.cmake-format.py`](https://github.com/openblack/openblack/blob/master/.cmake-format.py) files.
Additionally, a bot will comment on any issues found in your PRs, ensuring you receive immediate feedback on your code.


### File Names

Header files should be in CamelCase, should not have underscores and should end with `.h` instead of `.hpp`.

Example:
```
Correct: MyHeader.h
Incorrect: my_header.hpp
```

### Include Groups

To ensure consistent and organized include directives, we follow the include order guidelines based on the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Names_and_Order_of_Includes).

The include order should follow the following groups:

- **C Library Includes**: Include headers from the C library (e.g., `<cstdio>`, `<cmath>`).
- **Standard Library Includes**: Include headers from the C++ standard library (e.g., `<iostream>`, `<string>`).
- **Third party Library Includes**: Include headers from external libraries used in the project.
- **Project-Specific Includes**: Include headers from the current project.
- **Local Includes**: Include headers from the current project's directories.

For example:

```cpp
#include <cstdio>
#include <cmath>

#include <iostream>
#include <string>

#include <external_library/ExternalHeader.h>

#include "ProjectSpecificHeader.h"
#include "LocalHeader.h"
```

### Brace Style

The brace style used in openblack is the Allman style (also known as "GNU style" or "BSD style") for both control structures and function declarations.

Example:

```cpp
if (condition)
{
    // Code
}
else
{
    // Code
}

void myFunction()
{
    // Code
}
```

### Column Limit and Maximum Empty Lines

The column limit is set to 128 characters. This means that each line of code should not exceed this limit.
We also prefer that comments be limited to 80 characters to be concise and reduce scrolling.
Keeping lines within the column limit improves code readability and avoids horizontal scrolling in editors.
Additionally, we allow a maximum of one consecutive empty line to be kept. Excessive empty lines can make the codebase appear disorganized and can hinder readability.

### Copyright Statement

Each source file in openblack must include the same copyright statement. If a source file is taken from the project it must comply with the license and include these statement.

### The `openblack` Namespace

To prevent naming conflicts and ensure code isolation, openblack code resides within the `openblack` namespace.
Additional namespace nesting is often used for different subsections of the engine (e.g. `openblack::graphics`, `openblack::ecs`, `openblack::resources`).

For example:

```cpp
// MyClass.h
namespace openblack::subsection {

class MyClass {
  MyClass::MyClass();
};

} // namespace openblack

// MyClass.cpp file
using namespace openblack::subsection;
MyClass::MyClass() = default;

// Other .cpp file
using namespace openblack;
std::make_unique<subsection::MyClass>();
```

By encapsulating declarations within the `openblack` namespace, we minimize the risk of clashes with other codebases and provide a clear and distinct identity for openblack.

### [Naming Conventions](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html):

- Global constants should be in CamelCase and prefixed with `k_`.
- Methods should be in CamelCase.
- Variables and parameters should be in camelBack (lowerCamelCase) notation. The prefix `u_` is reserved for uniforms in shaders.
- Class members should be in camelBack notation, and private/protected members should be prefixed with `_`.

Examples:
```
Global Constant: k_GlobalConstant
Method: MyMethod
Variable: myVariable, u_uniform
Class Member: myClassMember, _privateClassMember
```

### Indentation:

- C++ files: We use tabs of size 4 for indentation, not spaces.
- CMake Files: We use 2 spaces to indent.

### [Cognitive Complexity](https://clang.llvm.org/extra/clang-tidy/checks/readability/function-cognitive-complexity.html)

We aim to keep functions as simple as possible.

Cognitive complexity is a measure of how difficult a code section is to intuitively understand. Unlike cyclomatic complexity, it isn't determined solely by the structure of the code but also considers the code's readability aspects.

Here is an example function with a cognitive complexity of 3:

```cpp
// Cognitive complexity of this function is 3.
int calculateValue(int a, int b, bool condition)
{
    if (condition)       // +1 for the if condition
    {
        while (a < b)    // +1 for the loop
        {
            a += 2;
            if (a == b)  // +1 for the second if condition
            {
                break;
            }
        }
    }
    return a;
}
```

### Short Statements

Short blocks, short case labels, short functions, and short if statements are not allowed on a single line. Short loops are not allowed on a single line either.

Example:
```cpp
// Incorrect
for (int i = 0; i < 10; ++i) increment(i);

// Correct
for (int i = 0; i < 10; ++i)
{
    increment(i);
}
```

### Mutability and `auto`

We adhere to a "const-by-default" principle, using `const` to denote that a variable should not be modified after it has been initialized, unless it needs to be mutable.

This communicates intent clearly and helps avoid bugs by preventing inadvertent modification of variables.

When declaring variables, we prefer placing `const` before the type rather than after it. This ensures consistency and clarity.

We encourage the use of `auto` for variable declarations, as it offers several benefits:

- **Type Inference**: `auto` allows the compiler to automatically deduce the type of the variable from its initializer, making the code cleaner and easier to read.
    While it is true that modern IDEs provide type information, using `auto` reduces redundancy and improves code maintainability, especially when working with complex types or templates.

- **Avoid Implicit Conversions**: Using `auto` can help prevent unintended implicit conversions.
    By deducing the type from the initializer, it ensures that the variable's type matches the expression's type exactly, reducing the risk of bugs caused by implicit conversions.

- **Improve Code Maintainability**: When the type in an expression changes, using `auto` eliminates the need to update the variable declaration explicitly.
    This leads to more maintainable code, as it reduces the likelihood of introducing errors during refactoring.

While there are differing opinions about the usage of `auto`, we believe that the benefits of improved code readability and maintainability outweigh the concerns about not explicitly specifying the type on the declaration line.
Modern IDEs indeed provide information about the inferred types, and code navigation tools make it easy to understand the context and type of variables.

### Header Management

Avoid using mega headers such as `entt/entt.hpp` which bring in more code than necessary and in turn slow down compilation times.
Especially in headers.

Instead, try using forward declaration in headers and including more fine-grained includes.

### Compiler and style warnings

All style warnings and any other compiler warnings are treated as errors in our upstream integration tests. So make sure your code doesn't generate any warnings.

## Architecture Guidelines

### Service Locator Pattern

openblack utilizes the service locator pattern provided by the [EnTT library](https://skypjack.github.io/entt/md_docs_md_locator.html).

The service locator pattern is a design pattern that enables the decoupling of systems and components by providing a centralized mechanism for accessing shared resources or services.

To use the service locator pattern, you can access resources by calling the appropriate methods on the `Locator` object. Here's an example:

```cpp

#include "Locator.h"             // required for Locator::resources
#include "Resources/Resources.h" // required for value().GetMeshes()
// ...
const auto& meshes = Locator::resources::value().GetMeshes();
```

By calling `value()`, we retrieve the instance of the resource locator, which allows us to access the `GetMeshes()` function.

By utilizing the service locator pattern, we can easily manage and access shared resources within the openblack codebase, ensuring efficient and decoupled resource handling.
We are also able to optimize our use of headers to reduce compile time memory usage and time by only including headers which are needed.

### Entity Component System (ECS)

We use the Entity Component System (ECS) architectural pattern, implemented with the [EnTT library](https://skypjack.github.io/entt/md_docs_md_entity.html), to achieve flexible and performant game development.

The ECS pattern is a [Data-Oriented Design](https://gamesfromwithin.com/data-oriented-design) (DOD) as opposed to Object oriented Design (OOD). This is a departure from the original Black & White game which made extensive use of OOD patterns such as deep and multiple inheritance.

The ECS pattern separates entities into their core components, allowing for efficient data processing and decoupling of logic. The EnTT library provides a lightweight and efficient implementation of ECS, making it an ideal choice for our project.

**Directories**

- `src/ECS/Components/`: The Components directory contains the individual component definitions.
    * Components represent the various attributes or properties of entities, such as position, velocity, or health.
    * Components are usually small, [passive structures](https://en.wikipedia.org/wiki/Passive_data_structure).
    * Typically, their behaviours are controlled by functions in `Systems` and their factory functions are in `Archetypes`.
    * By separating components, we achieve better data locality and enable systems to process relevant data efficiently.
- `src/ECS/Systems/`: The Systems directory contains the implementation of systems, which are responsible for processing and updating entities based on their components.
    * Systems encapsulate the game logic and interact with the components to modify or query entity data.
    * By dividing functionality into separate systems, we achieve better organization and maintainability of the codebase.
- `src/ECS/Archetypes/`: This directory contains archetypes, which are predefined entity templates or configurations that can be used to create entities with specific sets of components.
    * This concept is the one which most closely resembles OOD as it encapsulates multiple Components into one Archetype.
    * An example of an archetype is a Villager Archetype might describe and entity with the following components on them most of the time
        * `Villager`: Will be modified by the villager System
        * `WallHug`: Will be modified by the WallHug system (path finding), which includes the `Creature` component.
        * `Mesh`: Will be rendered by the rendering system along with all other meshes
        * `LivingAction`: Will go through the `LivingActionSystem` AI state machine to choose and perform actions.
        * etc
    * Archetypes are typically just static functions for creating entities with pre-defined components.
    * By defining archetypes, we can group higher level OOD concepts and avoid duplicate code.

By leveraging the EnTT library and the directory structure mentioned above, openblack benefits from the advantages of ECS, including improved performance, flexibility, and code maintainability.

### File Loaders

For each custom game file type (e.g., .lnd, .anm, .l3d files), we create loaders and writers located in the `components/` directory.

The loaders support both reading from file streams and memory byte streams, enabling us to load games directly from various sources such as CD CAB files, ISO archives, ZIPs, or other abstractions like in Emscripten and Android.

Example for a custom file type with extension `.ext`:
- components/ext/include/ExtFile.h
- components/ext/src/ExtFile.cpp
- apps/exttool/exttool.cpp

The `ExtFile.h` for each custom file type should have the following functions:

```cpp
// Read ext file from the filesystem
void Open(const std::filesystem::path& filepath);
// Read ext file from a buffer
void Open(const std::vector<uint8_t>& buffer);
// Write ext file to path on the filesystem
void Write(const std::filesystem::path& filepath);
```

Replace `ext` with the appropriate extension for each file type.

The components/ and apps/ directories show abundant examples of how to handle different types.
Please note that some file types are pack file types meaning that they contain other file types in their segments.
In such case, the file type should be interpreted as pack file that pipes into another type of file.
This pattern allows us to extract types for analysis, modding and other manipulations.
