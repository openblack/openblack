find_package(Python3 COMPONENTS Interpreter)

add_executable(gen_info gen_info.cpp)
target_include_directories(gen_info PRIVATE ../../src)
target_link_libraries(gen_info PRIVATE glm::glm cxxopts::cxxopts)
set_property(TARGET gen_info PROPERTY FOLDER "tests")
if (MSVC)
  target_compile_options(gen_info PRIVATE /utf-8)
endif ()

set(AUDIO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Audio)
set(AUDIO_PLACEHOLDER_OUTPUT ${AUDIO_BINARY_DIR}/placeholder)

set(DATA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)
set(CONE_MESH_OUTPUT ${DATA_BINARY_DIR}/cone.l3d)
set(CONE_INPUT ${DATA_BINARY_DIR}/cone.gltf)
set(MARKER_MESH_OUTPUT ${DATA_BINARY_DIR}/marker.l3d)
set(MARKER_INPUT ${DATA_BINARY_DIR}/marker.gltf)
set(RIVER_MESH_OUTPUT ${DATA_BINARY_DIR}/river.l3d)
set(RIVER_INPUT ${DATA_BINARY_DIR}/river.gltf)
set(RIVER2_MESH_OUTPUT ${DATA_BINARY_DIR}/river2.l3d)
set(RIVER2_INPUT ${DATA_BINARY_DIR}/river2.gltf)
set(SPHERE_MESH_OUTPUT ${DATA_BINARY_DIR}/metre_sphere.l3d)
set(SPHERE_INPUT ${DATA_BINARY_DIR}/metre_sphere.gltf)
set(CAM_CAM_OUTPUT ${DATA_BINARY_DIR}/cam.cam)
set(FLYING_CAM_OUTPUT ${DATA_BINARY_DIR}/flying.cam)

set(TEXTURES_BINARY_DIR ${DATA_BINARY_DIR}/Textures)
list(APPEND RAW_TEXTURES_0x10000_ARRAY ${TEXTURES_BINARY_DIR}/misc0a.raw)
list(APPEND RAW_TEXTURES_0x10000_ARRAY ${TEXTURES_BINARY_DIR}/smallbumpa.raw)
list(APPEND RAW_TEXTURES_0x10000_ARRAY ${TEXTURES_BINARY_DIR}/Skya.raw)
string(REPLACE ";" "," RAW_TEXTURES_0x10000 "${RAW_TEXTURES_0x10000_ARRAY}")
list(APPEND RAW_TEXTURES_0x30000_ARRAY ${TEXTURES_BINARY_DIR}/Sky.raw)
string(REPLACE ";" "," RAW_TEXTURES_0x30000 "${RAW_TEXTURES_0x30000_ARRAY}")

set(CITADEL_BINARY_DIR ${DATA_BINARY_DIR}/Citadel)
set(CITADEL_OUTSIDE_BINARY_DIR ${CITADEL_BINARY_DIR}/OutsideMeshes)
set(CITADEL_ENGINE_BINARY_DIR ${CITADEL_BINARY_DIR}/engine)
set(CITADEL_ENGINE_CAM_OUTPUT ${CITADEL_ENGINE_BINARY_DIR}/test.cam)

set(ALL_MESHES_OUTPUT ${DATA_BINARY_DIR}/AllMeshes.g3d)
set(ALL_ANIMATIONS_OUTPUT ${DATA_BINARY_DIR}/AllAnims.anm)

set(INTERMEDIATE_DATA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Intermediate)
set(INTERMEDIATE_MESH_BINARY_DIR ${INTERMEDIATE_DATA_BINARY_DIR}/Meshes)
set(DUMMY_MESH_FILE ${INTERMEDIATE_MESH_BINARY_DIR}/Dummy.gltf)
set(DUMMY_MESH_OUTPUT ${INTERMEDIATE_MESH_BINARY_DIR}/Dummy.l3d)
set(ABODE_MESH_FILE ${INTERMEDIATE_MESH_BINARY_DIR}/BuildingCeltic5A.gltf)
set(ABODE_MESH_OUTPUT ${INTERMEDIATE_MESH_BINARY_DIR}/BuildingCeltic5A.l3d)
set(VILLAGE_CENTRE_MESH_FILE
    ${INTERMEDIATE_MESH_BINARY_DIR}/BuildingCelticVillageCentre.gltf
)
set(VILLAGE_CENTRE_MESH_OUTPUT
    ${INTERMEDIATE_MESH_BINARY_DIR}/BuildingCelticVillageCentre.l3d
)

set(SYMBOLS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data/Symbols)
set(SYMBOLS_PLACEHOLDER_OUTPUT ${SYMBOLS_BINARY_DIR}/placeholder)
set(PATH_SYMBOL_1_CAM_OUTPUT ${SYMBOLS_BINARY_DIR}/PathSymbol1.cam)

set(CREATURE_MESH_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data/CreatureMesh)
set(HAND_BASE_OUTPUT ${CREATURE_MESH_BINARY_DIR}/Hand_Boned_Base2.l3d)
set(HAND_BASE_MESH_FILE ${CREATURE_MESH_BINARY_DIR}/Hand_Boned_Base2.gltf)

set(SKY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data/WeatherSystem)
set(SKY_OUTPUT ${SKY_BINARY_DIR}/sky.l3d)
set(SKY_MESH_FILE ${SKY_BINARY_DIR}/sky.gltf)

set(COFFRE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data/Misc)
set(COFFRE_MESH_OUTPUT ${COFFRE_BINARY_DIR}/coffre.l3d)
set(COFFRE_ANIM_OUTPUT ${COFFRE_BINARY_DIR}/coffre.anm)
set(COFFRE_INPUT ${COFFRE_BINARY_DIR}/coffre.gltf)

set(TERRAIN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data/Landscape)
set(TERRAIN_MULTIPLAYER_BINARY_DIR ${TERRAIN_BINARY_DIR}/Multi_Player)
set(TERRAIN_TYPE 0)
set(TERRAIN_LAND_1_OUTPUT ${TERRAIN_BINARY_DIR}/Land1.lnd)
set(TERRAIN_LAND_MPM_2P_1_OUTPUT ${TERRAIN_MULTIPLAYER_BINARY_DIR}/MPM_2P_1.lnd)
set(TERRAIN_NOISE_MAP ${TERRAIN_BINARY_DIR}/noise_map.r8.raw)
set(TERRAIN_BUMP_MAP ${TERRAIN_BINARY_DIR}/bump_map.r8.raw)
list(APPEND TERRAIN_MATERIAL_ARRAY
     ${TERRAIN_BINARY_DIR}/material_data1.bgr5a1.raw
)
list(APPEND TERRAIN_MATERIAL_ARRAY
     ${TERRAIN_BINARY_DIR}/material_data2.bgr5a1.raw
)
list(APPEND TERRAIN_MATERIAL_ARRAY
     ${TERRAIN_BINARY_DIR}/material_data3.bgr5a1.raw
)
string(REPLACE ";" "," TERRAIN_MATERIAL_LIST "${TERRAIN_MATERIAL_ARRAY}")

set(SCRIPTS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Scripts)
set(QUESTS_BINARY_DIR ${SCRIPTS_BINARY_DIR}/Quests)
set(INFO_UNPACKED_OUTPUT ${SCRIPTS_BINARY_DIR}/Info)
set(INFO_DAT_OUTPUT ${SCRIPTS_BINARY_DIR}/info.dat)
set(QUEST_CHALLENGE_OUTPUT ${QUESTS_BINARY_DIR}/challenge.chl)

# TODO: Combine all texture generating scripts into a single one

add_custom_target(
  generate_mock_game_data
  COMMAND ${CMAKE_COMMAND} -E make_directory ${AUDIO_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DATA_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TEXTURES_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CITADEL_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CITADEL_OUTSIDE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CITADEL_ENGINE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CREATURE_MESH_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SKY_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TERRAIN_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TERRAIN_MULTIPLAYER_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COFFRE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SCRIPTS_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${QUESTS_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SYMBOLS_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${INTERMEDIATE_DATA_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${INTERMEDIATE_MESH_BINARY_DIR}
  # Add placeholders so that the directories get copied
  COMMAND ${CMAKE_COMMAND} -E touch ${AUDIO_PLACEHOLDER_OUTPUT}
  # Copy the Scripts Directory
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Scripts
          ${CMAKE_CURRENT_BINARY_DIR}/Scripts
  # Generate Mock Raw Textures
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py
          --output-files "${RAW_TEXTURES_0x10000}" --size 0x10000
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py
          --output-files "${RAW_TEXTURES_0x30000}" --size 0x30000
  # Generate Citadel files
  COMMAND ${CMAKE_COMMAND} -E touch ${CITADEL_OUTSIDE_BINARY_DIR}/placeholder
  # Data/AllMeshes.g3d
  # Dummy mesh for 626 meshes
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${DUMMY_MESH_FILE}
  COMMAND l3dtool write -o ${DUMMY_MESH_OUTPUT} -i ${DUMMY_MESH_FILE}
  COMMAND
    Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py --output-file
    ${ABODE_MESH_FILE} --translate-positions 0.0883901119,0.0,0.188308239
    --scale-positions 6.33014917,1.0,6.33014917
  COMMAND
    Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py --output-file
    ${VILLAGE_CENTRE_MESH_FILE} --translate-positions
    0.00188350677,0.0,0.22130537 --scale-positions 12.9809408,1.0,12.9809408
  COMMAND l3dtool write -o ${ABODE_MESH_OUTPUT} -i ${ABODE_MESH_FILE}
  COMMAND l3dtool write -o ${VILLAGE_CENTRE_MESH_OUTPUT} -i
          ${VILLAGE_CENTRE_MESH_FILE}
  COMMAND packtool --write-mesh ${ALL_MESHES_OUTPUT} ${DUMMY_MESH_OUTPUT}:626
          ${ABODE_MESH_OUTPUT}:79:1 ${VILLAGE_CENTRE_MESH_OUTPUT}:166:1
  # Data/AllAnims.anm
  COMMAND packtool --write-animation ${ALL_ANIMATIONS_OUTPUT}
  # Data/CreatureMesh/Hand_Boned_Base2.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${HAND_BASE_MESH_FILE}
  COMMAND l3dtool write -o ${HAND_BASE_OUTPUT} -i ${HAND_BASE_MESH_FILE}
  # Data/Landscape/Land1.lnd
  COMMAND
    Python3::Interpreter
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Landscape/gen_land_textures.py --output-dir
    ${TERRAIN_BINARY_DIR}
  COMMAND
    lndtool write -o ${TERRAIN_LAND_1_OUTPUT} --terrain-type ${TERRAIN_TYPE}
    --noise-map ${TERRAIN_NOISE_MAP} --bump-map ${TERRAIN_BUMP_MAP}
    --material-array=${TERRAIN_MATERIAL_LIST} --points
    "1788.40 28.9173050 2710.00"
  COMMAND
    lndtool write -o ${TERRAIN_LAND_MPM_2P_1_OUTPUT} --terrain-type
    ${TERRAIN_TYPE} --noise-map ${TERRAIN_NOISE_MAP} --bump-map
    ${TERRAIN_BUMP_MAP} --material-array=${TERRAIN_MATERIAL_LIST}
  # Data/WeatherSystem/sky.l3d
  COMMAND
    Python3::Interpreter
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/WeatherSystem/gen_sky.py --output-dir
    ${SKY_BINARY_DIR}
  COMMAND l3dtool write -o ${SKY_OUTPUT} -i ${SKY_MESH_FILE}
  # Data/Misc/coffre.{l3d,anm}
  COMMAND
    Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/Data/Misc/gen_coffre.py
    --output-dir ${COFFRE_BINARY_DIR}
  COMMAND l3dtool write -o ${COFFRE_MESH_OUTPUT} -i ${COFFRE_INPUT}
  COMMAND anmtool write -o ${COFFRE_ANIM_OUTPUT} -i ${COFFRE_INPUT}
  # Data/cone.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${CONE_INPUT}
  COMMAND l3dtool write -o ${CONE_MESH_OUTPUT} -i ${CONE_INPUT}
  # Data/marker.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${MARKER_INPUT}
  COMMAND l3dtool write -o ${MARKER_MESH_OUTPUT} -i ${MARKER_INPUT}
  # Data/river.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${RIVER_INPUT}
  COMMAND l3dtool write -o ${RIVER_MESH_OUTPUT} -i ${RIVER_INPUT}
  # Data/river2.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${RIVER2_INPUT}
  COMMAND l3dtool write -o ${RIVER2_MESH_OUTPUT} -i ${RIVER2_INPUT}
  # Data/metre_sphere.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          --output-file ${SPHERE_INPUT}
  COMMAND l3dtool write -o ${SPHERE_MESH_OUTPUT} -i ${SPHERE_INPUT}
  # Data/cam.cam
  COMMAND camtool write -o ${CAM_CAM_OUTPUT} -s 0 --points
          "0.0 0.0 0.0 0.0 0.0 0.0"
  # Data/flying.cam
  COMMAND camtool write -o ${FLYING_CAM_OUTPUT} -s 0 --points
          "0.0 0.0 0.0 0.0 0.0 0.0"
  # Data/Symbols/PathSymbol1.cam
  COMMAND camtool write -o ${PATH_SYMBOL_1_CAM_OUTPUT} -s 0 --points
          "0.0 0.0 0.0 0.0 0.0 0.0"
  # Data/Citadel/engine/challenge.cam
  COMMAND camtool write -o ${CITADEL_ENGINE_CAM_OUTPUT} -s 0 --points
          "0.0 0.0 0.0 0.0 0.0 0.0"
  # Scripts/info.dat
  COMMAND gen_info --output-file "${INFO_UNPACKED_OUTPUT}"
  COMMAND packtool --write-raw ${INFO_DAT_OUTPUT} ${INFO_UNPACKED_OUTPUT}
  # Scripts/Quests/challenge.chl
  COMMAND
    Python3::Interpreter
    ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Quests/gen_challenge.py --output-file
    ${QUEST_CHALLENGE_OUTPUT}
  COMMENT "Generating mock game data in ${CMAKE_CURRENT_BINARY_DIR}"
  BYPRODUCTS ${HAND_BASE_MESH_FILE}
             ${SKY_MESH_FILE}
             ${DUMMY_MESH_FILE}
             ${DUMMY_MESH_OUTPUT}
             ${ABODE_MESH_FILE}
             ${ABODE_MESH_OUTPUT}
             ${VILLAGE_CENTRE_MESH_FILE}
             ${VILLAGE_CENTRE_MESH_OUTPUT}
             ${TERRAIN_NOISE_MAP}
             ${TERRAIN_BUMP_MAP}
             ${TERRAIN_MATERIAL_ARRAY}
             ${RAW_TEXTURES}
             ${COFFRE_INPUT}
             ${CONE_INPUT}
             ${MARKER_INPUT}
             ${RIVER_INPUT}
             ${RIVER2_INPUT}
             ${SPHERE_INPUT}
  SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
          ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py
          ${CMAKE_CURRENT_SOURCE_DIR}/Data/Landscape/gen_land_textures.py
          ${CMAKE_CURRENT_SOURCE_DIR}/Data/WeatherSystem/gen_sky.py
)
set_property(TARGET generate_mock_game_data PROPERTY FOLDER "tests")
