# Polyphonic Synthesizer

## Overview

![keyboard](Journal/img_11.png)

You're probably wondering why this is called a polyphonic synthesizer. Well, I thought it sounded fancier than "scrappy 2 octave piano". Anyway, this is my 2nd PCB project, made for the week 3, Instruments, of the [Half Life YSWS](halflife.hackclub.com) from Hack Club.

As a former unskilled piano player (AMEB G6), I decided to make an electronic keyboard this week cos why not. :))) 

Well, at least it's supposed to be an electric keyboard. I'm pretty sure the keys are a bit smaller than they actually are supposed to be, but at least their shape is recognisable, right?

## Schematics

![schematic image](Journal/img_4.png)

Above is my final schematic for the Synthesizer. As you can see, it contains a USB-C receptacle, Raspberry Pi Pico, some OLEDs, 25 switches (1 per key for 25 keys), and 8 knobs for customised sound settings.

## Hardware and assembly

![render](Journal/img_8.png)

This is my PCB render of my project. 

Basically, you need a bunch of Cherry MX switches that you need to stick onto the squares at the bottom. 

Then, you gotta 3D print the white and black key files in [here](3D%20Models/STLs/), ideally in PLA or PETG filament. 

Next, you need to stick the holes of the 3D printed parts onto the stems of the switches, and there should be enough tolerance to stick it in easily.

There's also a bunch of soldering that you need to do for the electrical components, but aside from that it's pretty easy.

![enclosure](Journal/img_12.png)

It is also important to note that the [enclosure](3D%20Models/STLs/enclosure.stl) is completely optional, and exists only to hold the PCB so it doesn't slide around if you're trying to play it. It does not in any way affect the functionality of the actual piano.

If you do print the enclosure, please note that it is too large for a standard 3D printer with a buildspace of 220mm x 220mm x 220mm, so I have already halved the box in the STL model, and all you gotta do is either superglue the halves together or zip tie the 2 halves together using the provided 6mm diameter zip tie holes. (Zip ties are recommended)

The enclosure also has 2 holes for the USB cable and also the audio jack. Place the PCB inside it accordingly. 

You may also notice that there is absolutely no top panel or lid for the enclosure. Please note that this is **intended**. so you can see my beautiful silkscreen art and also I do not have any access to hinges :CC

## Features

I dont even have much to say here, it's just a regular piano?? If you're seeing this and you're a music student then this is a sign to go back to studying music theory.

- Optional Ear/Headphone support (there's an audio jack but you don't need to use it. unfortunately no bluetooth support :C)
- Feels like an actual electronic keyboard (cos of the keyswitches =DDD)
- Adjustable knobs (edit: im done programming firmware! :3)

| Knob | What it does                                                                             |
|---|------------------------------------------------------------------------------------------|
| **Volume** | how loud the thingy is                                                                   |
| **Instrument** | switches between 5 sounds: Piano, Organ, Pluck, Bell and 8-bit                           |
| **Tone** | makes it darker or brighter, like turning the treble down/up                             |
| **Attack** | how fast a note fades in, turn up for a swell similar to a violin                        |
| **Release** | basically a damper pedal                                                                 |
| **Echo** | adds repeating echoes                                                                    |
| **Octave** | shifts the keyboard +/- 2 octaves so you can basically get the range of the entire piano |
| **Transpose** | shifts every note +/- 6 semitones in case you get bored of your key                      |

- The OLED shows the current instrument, octave and transpose, a bar when you turn a knob, and the notes you're holding.

## Firmware

The Firmware for this project is programmed in Arduino IDE.

## AI Usage

No AI used in this project.