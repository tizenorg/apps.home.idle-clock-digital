CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# install edj
ADD_CUSTOM_TARGET(idle-clock-digital.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
		${CMAKE_CURRENT_SOURCE_DIR}/idle-clock-digital.edc idle-clock-digital.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/idle-clock-digital.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} idle-clock-digital.edj)
INSTALL(FILES idle-clock-digital.edj DESTINATION ${EDJDIR})

# install images
#INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/images/ DESTINATION ${IMAGEDIR} FILES_MATCHING PATTERN "*.png")
