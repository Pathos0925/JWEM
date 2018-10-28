# JWEM
Jurassic World Evolution Modkit

Update dinosaur textures at runtime.

This is a proof of concept built on ReShade for replacing textures in DirectX games by hashing them at creation.

## How to use
 - Click on "Release" near the top of this page and download Release01.zip.
 - Extract the files and move them into your Jurassic World Evolution directory. 
 - Open the game.
 - Once in game, Hold SHIFT and press F2 to open the menu.
 - You may need change the Shader and/or Texture quality in the Settings.
	
	![JWEM UI](https://github.com/Pathos0925/JWEM/blob/master/ReadmeImages/JWEMmenu.png)
## Also
 - Any Dinosaurs you add should be detected automatically and added to the list. If you have a skin for that species, you should see a dropdown box that will let you select a new one.
- Currently, only BASIC SKINS are detected and changable. Additionally, any changes to the basic skin will affect every dinosaur with that skin.
- Add JWEMS skins to the skins folder. 
	- Included is a basic skin for Ankylosaurus called Ankylosaurus Blue. After loading it, you should see a blue Ankylosaurus.
	![BLUE ANKY](https://github.com/Pathos0925/JWEM/blob/master/ReadmeImages/BlueAnky.png)
	
## Creating Skins

### For Sharing
 - If you plan on making a skin pack (that can include albedo, normalmap, and other textures), use the Skin Builder.
 - Click "Add" and select your modified albedo texture.
 - Select the target species in the top right.
 - Click "Save".
 - Leave "Use GPU" Selected as using the CPU to compress DDS textures is very slow.
 - Thats it, drop it in the skins folder!
 
### For Testing
 - This method is better if you plan on repeatedly loading a texture. Supports several formats like DDS and PNG
 - Open SKIN_IDS.txt and find the species you want, name your file in the following format "You_skin_name_here_ID-RESOLUTION-TEXTURETYPE.png"
 - If you're loading an Albedo texture for an Allosaurus, it might look like: Test1_ALLO-0-1024-A.PNG
 - Or: RedTyranno_TYRA-0-1024-A.DDS
 - Place the texture in the Skins folder and click "Reload Skins" in the UI.
 - Higher resolutions are only found on a few dinosaurs, I think the large sauropods, so only use a higher resolution if your target resolution requires it.
  - This method compresses the image on the GPU so it may take a few moments each time you load it. 

## Also
 - At preasent, JWEM will likely lower your framerate significantly, you may need to lower your graphics settings. However you may need to keep Shader and Texture settings on medium or high.
 - Any DLL injector should work, so long as you inject it right as the game starts.
 - If you find loading a save with many dinosaurs takes too long, uncheck "Enabled" in the in-game menu until after you load the save and recheck it before you release a dinosaur.
 
## Questions
 - Where do I find textures for a dino?
 	- Search Google, theres a few methods.
- Can I use ReShade with this?
	- I dunno, maybe. Let me know what you find.

## TODO
stuff
