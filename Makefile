#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ledc

include $(IDF_PATH)/make/project.mk

compile_commands.json:
	+bear --use-cc $(CC) $(MAKE)
