SRC       = src
BUILD     = build

CPP       = g++.exe
CC        = gcc.exe
WINDRES   = windres.exe
OBJ       = $(BUILD)/data_processing.o $(BUILD)/device_test.o $(BUILD)/libchaos.o $(BUILD)/usb_comm.o $(BUILD)/peaks.o
LIBS      = libusb.a
BIN       = libchaos.a
CXXFLAGS  = -Wall -O2 -s
LDFLAGS   = -s -Wl,--gc-sections -Os
GPROF     = gprof.exe
RM        = rm -f
MKDIR     = mkdir
LINK      = ar

.PHONY: all all-before all-after clean clean-custom
all: all-before "$(BUILD)/$(BIN)" all-after

clean: clean-custom
	$(RM) $(OBJ) "$(BUILD)/$(BIN)"

all-before:
	test -d $(BUILD) || mkdir $(BUILD)

"$(BUILD)/$(BIN)": $(OBJ)
	$(LINK) rcu "$(BUILD)/$(BIN)" $(OBJ)

$(BUILD)/data_processing.o: $(GLOBALDEPS) $(SRC)/data_processing.cpp $(SRC)/data_processing.h $(SRC)/libchaos.h
	$(CPP) -c $(SRC)/data_processing.cpp -o $(BUILD)/data_processing.o $(CXXFLAGS)

$(BUILD)/device_test.o: $(GLOBALDEPS) $(SRC)/device_test.cpp $(SRC)/device_test.h $(SRC)/libchaos.h $(SRC)/usb_comm.h $(SRC)/usb_commands.h
	$(CPP) -c $(SRC)/device_test.cpp -o $(BUILD)/device_test.o $(CXXFLAGS)

$(BUILD)/libchaos.o: $(GLOBALDEPS) $(SRC)/libchaos.cpp $(SRC)/libchaos.h $(SRC)/usb_comm.h $(SRC)/usb_commands.h $(SRC)/libchaos.h $(SRC)/device_test.h $(SRC)/libchaos.h $(SRC)/usb_comm.h $(SRC)/data_processing.h $(SRC)/libchaos.h $(SRC)/peaks.h $(SRC)/libchaos.h $(SRC)/usb_comm.h $(SRC)/data_processing.h
	$(CPP) -c $(SRC)/libchaos.cpp -o $(BUILD)/libchaos.o $(CXXFLAGS)

$(BUILD)/usb_comm.o: $(GLOBALDEPS) $(SRC)/usb_comm.cpp $(SRC)/usb_comm.h $(SRC)/usb_commands.h $(SRC)/libchaos.h
	$(CPP) -c $(SRC)/usb_comm.cpp -o $(BUILD)/usb_comm.o $(CXXFLAGS)

$(BUILD)/peaks.o: $(GLOBALDEPS) $(SRC)/peaks.cpp $(SRC)/peaks.h $(SRC)/libchaos.h $(SRC)/usb_comm.h $(SRC)/usb_commands.h $(SRC)/data_processing.h
	$(CPP) -c $(SRC)/peaks.cpp -o $(BUILD)/peaks.o $(CXXFLAGS)
