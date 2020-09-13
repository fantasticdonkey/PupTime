
Introduction
============

No job is too big, no pup is too small!

This is a simple Arduino sketch; a peculiar attempt at creating a (non-waterproof) watch that can accompany you on your runs.

Code runs on a `M5StickC <https://m5stack.com/products/stick-c>`_ based on the popular ESP32-PICO microcontroller. It it dependent on the the `M5StickC GitHub library <https://github.com/m5stack/M5StickC>`_. 

.. image:: docs/PupTime.JPG

Prerequisites
---------------

* Ensure you follow the `Quick Start for the M5StickC <https://docs.m5stack.com/#/en/arduino/arduino_development>`_ to have a working Arduino environment, with access to the libraries.
* The 80x80 images are stored in the separate :code:`PipTimeBitmaps.h` file. How to produce your own is described later in this document.

Hardware
---------------

`M5StickC <https://docs.m5stack.com/#/en/arduino/arduino_development>`_ development boards are equipped with a number of devices and sensors, including:

* ESP32 mictocontroller (240MHz dual core, 520KB SRAM, support for Wi-Fi and Bluetooth)
* Flash memory 4MB
* 80*160 colour LCD screen (ST7735S)
* 2 buttons
* Red LED
* Motion tracking sensor (MPU6886)
* Infrared transmittor
* Microphone (SPM1423)
* Real-time clock (BM8563)
* Power management (AXP192)
* Built-in rechargeable LiPo battery (95 mAh)

PupTime currently only uses a number of these, namely the buttons, real-time clock and the LCD screen.

.. image:: docs/M5StickC.JPG
	:width: 400

Installation
---------------

* If the RTC of your ESP32 has not been set before, you will want to give it the correct time the first time you run.

Example - Set date and time for the first time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    firstTimeSetup(); // Uncomment this line. Don't forget to re-comment it out afterwards, so date / time is not reset after every reboot.

    // Change your time values to actual time
    TimeStruct.Hours = 04;
    TimeStruct.Minutes = 41;
    TimeStruct.Seconds = 30;

    // Change your date values to actual date
    DateStruct.WeekDay = 0;
    DateStruct.Month = 8;
    DateStruct.Date = 23;
    DateStruct.Year = 2020;

You should not have to reset the date / time often, as the real-time clock should continue to maintain the current date and time, even if the device is powered off.

* Upload the INO file to your M5StickC.

Uploading your own images
=========================

The site `image2cpp <https://javl.github.io/image2cpp/>`_ can be used to convert your own images. 


Blog Post(s)
=========================

The project is described further in the following `Rosie the Red Robot <https://www.rosietheredrobot.com>`_ blog post:

* Coming soon!

Further Documentation
=========================

* `M5StickC <https://m5stack.com/products/stick-c>`_
* `M5StickC GitHub library <https://github.com/m5stack/M5StickC>`_
* `image2cpp <https://javl.github.io/image2cpp/>`_
