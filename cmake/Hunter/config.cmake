hunter_config(glm
  VERSION ${HUNTER_glm_VERSION}
  CMAKE_ARGS GLM_TEST_ENABLE=OFF)

hunter_config(EnTT
	VERSION 3.1.0
	URL "https://github.com/skypjack/entt/archive/v3.1.0.tar.gz"
	SHA1 "eb59eef176d934a5ea35c4160f5f87b573d850aa")
