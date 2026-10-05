# openblack

openblack reimplements Black & White (2001). Follow the conventions below for all C++ code and game design logic.

## Modern C++

The code base is C++20. Write modern, idiomatic C++:

- Own resources with RAII: `std::unique_ptr`/`std::shared_ptr` and value types, never raw `new`/`delete`.
- Use `std::optional` for values that may be absent, `std::span` for views over contiguous data, `std::array` over C
  arrays, `enum class` over plain enums.
- Prefer `<algorithm>` and `std::ranges` over hand-written loops where they read better.
- Use `constexpr` for constants (named `k_PascalCase`), designated initializers for aggregates, and `[[nodiscard]]` on
  getters and pure functions.
- Keep pure logic (formulas, state machines) free of global state so it can be unit tested with fakes, never with the
  real game data.

## EnTT

Game state and services use [EnTT](https://github.com/skypjack/entt):

- **ECS.** Entities live in the registry (`ecs::Registry`, `Locator::entitiesRegistry`). Their data goes in components:
  plain structs in `src/ECS/Components`. Entities are made by archetypes in `src/ECS/Archetypes`. Behaviour goes in
  systems: an interface in `src/ECS/Systems/<Name>SystemInterface.h` and an implementation in
  `src/ECS/Systems/Implementations`, guarded by `LOCATOR_IMPLEMENTATIONS` so that only `Locator.cpp` (and tests that
  build their own locator) include it. Data belonging to an entity, such as a player's alignment, is a component on that
  entity rather than a field of a system.
- **Service locator.** Systems and services are reached through `Locator` (`entt::locator`, declared in `src/Locator.h`
  and emplaced in `src/Locator.cpp`), by their interface. Don't add singletons or globals.
- **Resources.** Assets are loaded once through the resource caches (`Locator::resources`, loaders in
  `src/Resources/Loaders.cpp`) and looked up by `entt::hashed_string` ids. Pass `.value()` of a hashed string to
  `Contains`.

Use these where they fit. A small value type or a pure function doesn't need to be a component or a system.
