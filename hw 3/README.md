# Nexus Games Catalog

A highly aesthetic, premium video game catalog web application built with a modern design system. This project was created to demonstrate dynamic UI interactions, glassmorphism design trends, and efficient DOM manipulation without the use of heavy frameworks.

## 🚀 Features

- **Premium UI/UX:** Sleek dark mode design with glassmorphism (frosted glass) elements and neon glowing accent orbs.
- **Micro-Animations:** Smooth hover states on game cards, staggered entrance animations, and floating background effects that make the page feel alive.
- **Dynamic Content Rendering:** Game data is rendered dynamically using JavaScript, making it easy to add or remove titles from the catalog.
- **Real-time Search & Filtering:** 
  - Instant text-based search to find games by title or tag.
  - Category filter buttons (All, RPG, Cyberpunk, Sci-Fi) to sort the catalog.
- **Custom Assets:** Includes high-quality AI-generated game cover art created specifically for this project.

## 💻 Tech Stack

- **HTML5:** Semantic structure and SEO best practices.
- **Vanilla CSS3:** Custom styling utilizing CSS Variables, Flexbox, Grid, CSS animations (`@keyframes`), and backdrop-filters for the glass effect. Uses the *Outfit* Google Font for modern typography.
- **Vanilla JavaScript:** ES6+ logic for rendering the game grid and handling the search/filter event listeners. No external libraries or dependencies required.

## 📁 Project Structure

```text
hw 3/
└── video-game-catalog/
    ├── index.html        # Main HTML layout
    ├── style.css         # All styles, animations, and responsive rules
    ├── script.js         # Logic for rendering cards and search/filtering
    └── images/           # Directory containing the generated game covers
```

## 🛠️ How to Run Locally

Because the project fetches resources and uses modern ES6 structure, it is best viewed using a local web server. 

1. Open your terminal or command prompt.
2. Navigate to the project directory:
   ```bash
   cd "video-game-catalog"
   ```
3. Start a local Python HTTP server (requires Python installed):
   ```bash
   python -m http.server 8000
   ```
4. Open your web browser and navigate to:
   ```text
   http://localhost:8000
   ```

## 📝 Future Enhancements

- Shopping cart functionality.
- Individual game detail pages.
- Connecting to a backend database or REST API for fetching game data.
