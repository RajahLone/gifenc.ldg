# gifenc.ldg

Library using the LDG system and the GIF encoder functions from GIFLIB.

Used by:

* P2SM to save images in GIF format.
* MapEdit to save tiles to GIF (tiles singletons or tiles containor).
* MAKE_MAP to slice large GIF into tiles and map array.

Other programs can use it, please read these sources and P2SM GFA source.

# installation for makefiles

- pre-requisite: different targets of libldg.a in /opt/cross-mint/m68k-atari-mint/lib/

- in an empty folder,  
   ```mkdir ./build/68000```  
   ```mkdir ./build/68020```  
   ```mkdir ./build/ColdFire```  

- get /gifenc.ldg/ from [gifdec_r4_src.zip](https://ptonthat.fr/files/gifdec/gifdec_r4_src.zip) and unpack the contents to ./

- gifenc.ldg.xcodeproj is for Xcode 26.1, you may not need it if you use something else.
