cmake_minimum_required (VERSION 3.10)
project (leechcraft_azoth_standardstyles)
include (InitLCPlugin NO_POLICY_SCOPE)

LC_DEFINE_PLUGIN (
	SRCS
		standardstyles.cpp
		standardstylesource.cpp
	QT_COMPONENTS WebKitWidgets
	INSTALL_SHARE
	)
