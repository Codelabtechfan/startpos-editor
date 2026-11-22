# StartPos Editor (Geode mod for Geometry Dash 2.2091)

Add, save and remove start positions anywhere in any level, including main levels.

* **Online / your levels:** a green start-pos button appears next to the Play button.
* **Main levels (Stereo Madness etc.):** tap the "i" (info) button on the level page; the button is inside that info popup.
* **Editor:** drag the slider (or type an X position) to pick a spot, choose game mode, speed, mini and flipped gravity, then press **Add**.
  Press **Use** on any saved one to make it the one you start from (green = active), the red bin removes it, and **Play from start** turns it off.
* **In-game switcher:** press **Q** (previous) or **E** (next), or the on-screen arrows, to cycle start positions while playing, including starting from 0%. The label shows the current index (for example `2/5`). Turn it off, hide the arrows, rebind the keys, or change the idle opacity in the mod settings. **Use** also sets the switcher's current index.
* Everything is saved per level automatically.

## Get the .geode file (no programming needed)
1. Make a free GitHub account, create a new repository and upload all files from this folder (keep the `.github` folder!).
2. Open the **Actions** tab, then **Build Geode Mod**, then **Run workflow** (it also runs on every upload). Wait about 5-10 minutes.
3. Open the finished run and download **Build Output**. Unzip it to get `cookiegamer.startpos-editor.geode`.

## Install on Windows 11
1. Install Geode **v6.0.0-beta.1** (or later) from https://github.com/geode-sdk/geode/releases — this mod needs the Geode beta for Geometry Dash 2.2091. The stable installer on https://geode-sdk.org is still for 2.2081. Run the Windows installer and point it at your Geometry Dash folder.
2. Copy the `.geode` file into `Geometry Dash\geode\mods` (Steam: right-click GD, then Manage, then Browse local files).
3. Start the game.

## Building locally (optional, for developers)
Install the Geode CLI + SDK (`geode sdk install`), Visual Studio 2022 with C++ tools, CMake, then run `geode build`.

## Notes
* All saved start positions are loaded into the level when you play, so you can cycle them with the built-in switcher. **Use** (or **Play from start**) picks which one you begin at.
* Progress from start positions does not count, the same as normal start positions in GD.
