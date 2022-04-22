find_package (Python3 COMPONENTS Interpreter)

set(DATA_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Data)

set(TEXTURES_BINARY_DIR ${DATA_BINARY_DIR}/Textures)
list(APPEND RAW_TEXTURES_0x10000_ARRAY ${TEXTURES_BINARY_DIR}/misc0a.raw)
list(APPEND RAW_TEXTURES_0x10000_ARRAY ${TEXTURES_BINARY_DIR}/smallbumpa.raw)
string(REPLACE ";" "," RAW_TEXTURES_0x10000 "${RAW_TEXTURES_0x10000_ARRAY}")
list(APPEND RAW_TEXTURES_0x30000_ARRAY ${TEXTURES_BINARY_DIR}/Sky.raw)
string(REPLACE ";" "," RAW_TEXTURES_0x30000 "${RAW_TEXTURES_0x30000_ARRAY}")

set(CITADEL_BINARY_DIR ${DATA_BINARY_DIR}/Citadel)
set(CITADEL_OUTSIDE_BINARY_DIR ${CITADEL_BINARY_DIR}/OutsideMeshes)

set(ALL_MESHES_OUTPUT ${DATA_BINARY_DIR}/AllMeshes.g3d)
set(ALL_ANIMATIONS_OUTPUT ${DATA_BINARY_DIR}/AllAnims.anm)

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
set(TERRAIN_TYPE 0)
set(TERRAIN_OUTPUT ${TERRAIN_BINARY_DIR}/Land1.lnd)
set(TERRAIN_NOISE_MAP ${TERRAIN_BINARY_DIR}/noise_map.r8.raw)
set(TERRAIN_BUMP_MAP ${TERRAIN_BINARY_DIR}/bump_map.r8.raw)
list(APPEND TERRAIN_MATERIAL_ARRAY ${TERRAIN_BINARY_DIR}/material_data1.rgb5a1.raw)
list(APPEND TERRAIN_MATERIAL_ARRAY ${TERRAIN_BINARY_DIR}/material_data2.rgb5a1.raw)
list(APPEND TERRAIN_MATERIAL_ARRAY ${TERRAIN_BINARY_DIR}/material_data3.rgb5a1.raw)
string(REPLACE ";" "," TERRAIN_MATERIAL_LIST "${TERRAIN_MATERIAL_ARRAY}")

set(SCRIPTS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Scripts)
set(QUESTS_BINARY_DIR ${SCRIPTS_BINARY_DIR}/Quests)
set(INFO_UNPACKED_OUTPUT ${SCRIPTS_BINARY_DIR}/Info)
set(INFO_DAT_OUTPUT ${SCRIPTS_BINARY_DIR}/info.dat)
set(QUEST_CHALLENGE_OUTPUT ${QUESTS_BINARY_DIR}/challenge.chl)

# TODO: Combine all texture generating scripts into a single one

add_custom_target(generate-mock-game-data
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DATA_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TEXTURES_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CITADEL_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CITADEL_OUTSIDE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CREATURE_MESH_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SKY_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${TERRAIN_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${COFFRE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SCRIPTS_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${QUESTS_BINARY_DIR}
  # Copy the Scripts Directory
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Scripts ${CMAKE_CURRENT_BINARY_DIR}/Scripts
  # Generate Mock Raw Textures
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py --output-files "${RAW_TEXTURES_0x10000}" --size 0x10000
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py --output-files "${RAW_TEXTURES_0x30000}" --size 0x30000
  # Generate Citadel files
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py --output-files "${CITADEL_OUTSIDE_BINARY_DIR}/Entrance_l3d.zzz" --size 0x616
  # Data/AllMeshes.g3d
  COMMAND packtool --write-mesh ${ALL_MESHES_OUTPUT}
  # Data/AllAnims.anm
  COMMAND packtool --write-animation ${ALL_ANIMATIONS_OUTPUT}
  # Data/CreatureMesh/Hand_Boned_Base2.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py --output-file ${HAND_BASE_MESH_FILE}
  COMMAND l3dtool write -o ${HAND_BASE_OUTPUT} -i ${HAND_BASE_MESH_FILE}
  # Data/Landscape/Land1.lnd
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/Data/Landscape/gen_land_textures.py --output-dir ${TERRAIN_BINARY_DIR}
  COMMAND lndtool write -o ${TERRAIN_OUTPUT} --terrain-type ${TERRAIN_TYPE} --noise-map ${TERRAIN_NOISE_MAP} --bump-map ${TERRAIN_BUMP_MAP} --material-array=${TERRAIN_MATERIAL_LIST}
  # Data/WeatherSystem/sky.l3d
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/Data/WeatherSystem/gen_sky.py --output-dir ${SKY_BINARY_DIR}
  COMMAND l3dtool write -o ${SKY_OUTPUT} -i ${SKY_MESH_FILE}
  # Data/Misc/coffre.{l3d,anm}
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/Data/Misc/gen_coffre.py --output-dir ${COFFRE_BINARY_DIR}
  COMMAND l3dtool write -o ${COFFRE_MESH_OUTPUT} -i ${COFFRE_INPUT}
  COMMAND anmtool write -o ${COFFRE_ANIM_OUTPUT} -i ${COFFRE_INPUT}
  # Scripts/info.dat
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py --output-files "${INFO_UNPACKED_OUTPUT}" --size 0x8E186
  COMMAND packtool --write-raw ${INFO_DAT_OUTPUT} ${INFO_UNPACKED_OUTPUT}
  # Scripts/Quests/challenge.chl
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Quests/gen_challenge.py --output-file ${QUEST_CHALLENGE_OUTPUT}

  COMMENT "Generating mock game data in ${CMAKE_CURRENT_BINARY_DIR}"

  BYPRODUCTS
    ${HAND_BASE_MESH_FILE}
    ${SKY_MESH_FILE}
    ${TERRAIN_NOISE_MAP}
    ${TERRAIN_BUMP_MAP}
    ${TERRAIN_MATERIAL_ARRAY}
    ${RAW_TEXTURES}

  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_meshes.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_zeroed_file.py
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/Landscape/gen_land_textures.py
    ${CMAKE_CURRENT_SOURCE_DIR}/Data/WeatherSystem/gen_sky.py
)
