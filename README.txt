libchaos is the library which interacts with the chaos unit to gather
data. It use used by chaosconnect and could be used by other applications
which need to retrieve data from the chaos circuit.

This library can be built on Windows, Linux, and Mac OSX. It requires
libusb (or libusb-win32). GCC is required to compile the code. The
makefile is designed for Windows, so it may require modifications for
other platforms.

Acknowledgements

Thanks to Volodymyr Myrnyy for help on the FFT code
http://gfft.sourceforge.net/
http://www.ddj.com/cpp/199500857