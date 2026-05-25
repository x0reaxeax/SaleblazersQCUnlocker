# SaleblazersQCUnlocker
Unlocks hidden developer and debug commands within the in-game QuantumConsole.  

Required game version: v0.21.0.2  

## Warning
Many hidden commands are hidden on purpose, and can result in save-game / character data corruption.  
**USE AT YOUR OWN RISK**  

## Usage
1. BACKUP YOUR SAVE AND CHARACTER DATA
2. Inject `SaleblazersQCUnlocker.dll` into `Saleblazers.exe` process
  
## Features 
 - Patch for `BaseItemPlacingManager.CanPlaceObject` - Free Placement
 - Patches for QuantumConsole
   - Function patches 
     - `QuantumConsole.IsValidCommand`
     - `HRConsoleCommands.CheckAllowDeveloperCommand`
   - Patches for inline `["DEV"]` gated commands
   - Patch fix for give commands' `bypassLimit`/`Rarity` issue
  
  
## Config file
Config file allows for turning specific patch groups on/off:  
```
# Bypass placement restrictions (default: false)
BypassPlacementRestrictions=false

# Unlock Quantum Console dev mode (default: true)
UnlockQuantumConsoleDevMode=true

# Fix item give rarity bug (default: true)
ItemGiveRarityFix=true
```

## License
Copyright 2026 x0reaxeax

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
