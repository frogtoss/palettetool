# How to Export Palettes #

What follows is a guide to create palettes that can be used as input formats for Palette Tool using existing off the shelf tools.

## Lospec.com ##

Lospec has a database of RGB palettes with no alpha channels.  The individual colors are unnamed.

[Lospec](https://lospec.com/) is a great website to retrieve digitally restrictive palettes.  

 1. Browse to the site or [view the latest palettes](https://lospec.com/palette-list).
 2. Click on a specific palette.
 3. In the downloads section, "**choose PNG Image (1x)**"
 
Example command: 
 
    palettetool --in some-palette-1x.png --out some-palette.json 

## Photopea ##

[Photopea](https://www.photopea.com/), a Photoshop-like program is capable of exporting .aco files.  .aco is a file format created by Adobe that predates .ase.

Photopea's .aco files contain standard "version 1" data, which are nameless RGB files.  This is parsed by palettetool.  It contains a nonstandard second "version 2" data which cannot currently be parsed by palettetool.

 1. Open Photopea in your browser and choose a foreground color
 2. Select `Window > Swatches`
 3. From "**Color Picker**", click "**Add Swatch**".
 4. The "**Add name**" dialog will come up.  This name is not imported, so choose the default.

After repeating the above steps until you are satisfied with your palette:

 5. Click on the first colour, then click shift-click on the last colour you want to export.
 6. Right click and choose "Export as aco"
 
Exmaple command:

    palettetool --in some-palette.aco --out some-palette.json

## GNU Image Manipulation Program ##

NOTE: GPL input format is not yet implemented

Using GIMP v3, you can export .gpl format.  GPL export is planned for ftg_palettetool.

GPL format is RGB with no alpha channel.  While the colors appear to be named, there is no interface in GIMP to actually name them.

 1. Go to `Windows > Dockable Dialogs > Palettes`
 2. Click "**New Palette**" icon and name the new palette
 3. A new dialog opens with a big empty swatch list

At this point, you have the ability to set the number of columns.  This is meaningless for export purposes.  It can be set to one.

 4. With the desired foreground color, right click on the palette in the palette editor and choose  "**New Color from FG**" in the palette editor.
 5. Repeat this process until satisfied.
 6. Back out of the Palette Editor dialog into the Palette dialog.
 7. Right click and choose "refresh palettes". This seems to be necessary to save the palette to disk.
 8. Right click on the new palette and choose "**Show in File manager**".  Copy the new .gpl file to any chosen destination.
 
Example command:

    palettetool --in some-palette.gpl --out some-palette.json
    
# Programs That Don't Work With PaletteTool  #

This section lists programs that don't export to palettetool, as of this writing.  Added here to save you some time.

## Affinity Photo ##

Affinity Photo only exports `.afpalette`, which is [not an open format](https://forum.affinity.serif.com/index.php?/topic/88691-afpalette-format-various-issues/) and is subject to change and break.

## Krita ##

Krita exports [KPL files](https://docs.krita.org/en/general_concepts/file_formats/file_kpl.html) which are not yet supported. 
