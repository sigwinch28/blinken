#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ledc

include $(IDF_PATH)/make/project.mk

compile_commands.json:
	+bear --append --use-cc $(CC) $(MAKE)
	+bear --append --use-cc $(CC) $(MAKE) -C test
