
# Creating a Zephyr driver roadmap

1. Identify component to create a driver for
   - Done --> [Sparkfun Quad Alphanumeric display](https://www.sparkfun.com/sparkfun-qwiic-alphanumeric-display-blue.html)
2. Write functional code for the component (without making it a driver)
   - C code in progress
   - Arduino (C++) Libraries available for reference
3. Implement driver
   - Port original code to implement relevant auxdisplay functions
   - Ex. auxdisplay_write or auxdisplay_capabilities_get
   - Update Kconfig in drivers/auxdisplay
   - Update CMakeLists in drivers/auxdisplay
4. (Optional) Make unit tests using [ZTEST](https://docs.zephyrproject.org/latest/develop/test/ztest.html)
   - Test implemented functions
   - This is not necessary to make the pull request, but might be fun to learn
5. Create the pull request
   - Follow the [general guidelines](https://docs.zephyrproject.org/latest/contribute/index.html#general-guidelines)
   - Make sure code conforms to guidelines, optionally use [clang format](https://github.com/zephyrproject-rtos/zephyr/blob/main/.clang-format)
6. Cross fingers and wait
   - Wait for the pull request to be accepted or for feedback to be left
   - Fix code based on feedback if necessary then repeat step 5

