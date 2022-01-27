# name of your application
APPLICATION = Project
APPLICATION_VERSION = 0.1.0

# platform for which the application will be built
PLATFORM ?= octa

# Needs to be set to one to be able to print floats
PRINT_FLOATS = 1

# Add shields used in application
# Murata
SHIELDS += Murata-dualstack
MURATA_CONNECTOR = 3
LORAWAN_APPLICATION = lpiot

# Path to the octa-stack base directory:
STACKBASE ?= ../../..
FOLDER = examples

# Defined in Makefile.core:158
# Starts BLE UART => octa/src/octa.c:36
USE_BLE = 1

include $(STACKBASE)/core/Makefile.core