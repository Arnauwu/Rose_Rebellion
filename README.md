# Project 2 – CITM UPC

This repository is part of the **Project 2** at the **CITM (Center for Imaging and Multimedia Technology)**, Universitat Politècnica de Catalunya (UPC).

---

## 🎮 [Rose Rebellion]

In a world oppressed by a dragon, a princess with must fight the dragon to save her people and the kingdom.

---

## 📖 Description

**Rose Rebellion** is a 2D metroidvania and action game focused on combat, narrative, and collection.

---

## 🧱 Project Details

* **Team Members:**
    * González Olivé Arnau – Lead Game Designer, Narrative Designer, Lead Development, Level Designer - [Enlace al perfil](https://github.com/Arnauwu)
    * Yangfeng XU – Developer Gameplay, Developer Engineer - [Enlace al perfil](https://github.com/yangfeng-xu)
    * Corredor Mayol Jan– Developer Gameplay, Technical Artist, Level Designer, Developer Engineer - [Enlace al perfil](https://github.com/JanCorredor)
    * Jiménez Marquina Marc – Lead Game Designer, Developer Gameplay - [Enlace al perfil](https://github.com/MarcJM-00)
    * Mateo Guerrero Rubén – Producer, Lead Development, Developer Gameplay - [Enlace al perfil](https://github.com/Rubenmg0)
    * Wu Bole – Developer Gameplay - [Enlace al perfil](https://github.com/Bole05)

---

## 🚀 Release 

This release represents the vertical slice of the project. 

### 🌟 New Features

* **Complete UI System:** Implemented inventory and skill tree UI and a little map UI.
* **HUD Integration:** Added a new dynamic HUD displaying the player's health and dynamic HUD displaying diferents boss's health.
* **New Maps And Update Old Maps:** Creation of new maps, like mountain, new forest, catacombs. Each map with their Specefic music and objects, decorations, special door with their animation and special enemies.
* **Save & Load System:** Implemented persistence. Players can save their progress (F5) and continue from the last save point (F6), even after closing the game and with savepoint animation.
* **Player:** Added a **Vertical attack** **Wall jump** for the player. 
* **Audios:** New audios system, with fx sound to each entity and background music.
* **Enemies:** Implementation of new enemies dip, bat, cucafera shiny and 2 new boss, ninfa and dragon. And update old enemies textures and animations,
* **New Item interaction:** Proper management of existing items in the game, to provide diversity.
* **Dialogue System** Implemented a dialogue system with the NPC's to apport life to the game and know what happends in the history.
* **Cinematic system** Add a cinematic when enter the game to apport a principal idea for the gameplay.
* **Controller compatibility** Posibility to play the game with controller, without keyboard.
* **Effect particles** Implemented a particles to the player's action and enemies actions.

---

## 📦 Installation / Execution

1.  Download the latest `RELEASE` file from the Releases section.
2.  Extract all files to a folder.
3.  Run `RoseRebellion.exe`.

---

## 🧩 How to Play / Controls

### 🎮 Gameplay Controls

| Action | Key |
| :--- | :--- |
| Move Left | A |
| Move Right | D |
| Open door | W |
| Jump | Space |
| Attack | J |
| Wall jump | L |
| Dash | LCtrl |
| Glide | LShift |
| Open inventory | I |
| Open map | M |
| Open skill tree | N |
| Pause game | 0 |
| Open settings | ESC |

### 🧑‍💻 Debug Controls

Debug keys implemented to assist in testing features and states.

| Action | Key |
| :--- | :--- |
| Respawn to the last savepoint | R |
| Teleport to 0,0 | T |
| Add skill orb | P |
| Give yourself the Blanket (Glide) & Sickle (Attack) | 9 |
| View Colliders and Pathfinding routes | F9 |
| God Mode | F10 |

---

## 🔧 Optimizations & Fixes

### ⚡ Optimizations
* **Pathfiding A'*':** for a correct and better entities tracking.
* **Lazy Pathfiding:** for a stable use of memory.
* **Render Logic:** Optimized sprite batching and implemented rotated tiles to reduce memory usage.

### 🪲 Bug Fixes
* Use a lot of ram memory in walk in entities, that cause slight loss of frames.
* Some room transitions detect a false path leading to higher use of ram than expected, causes the game to be stoped some frames
---

## 🔗 Assets


© CITM – Universitat Politècnica de Catalunya
