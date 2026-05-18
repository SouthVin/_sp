const games = [
    {
        id: 1,
        title: "Neon City: Overdrive",
        category: "cyberpunk",
        tags: ["Cyberpunk", "Action", "Open World"],
        price: "$59.99",
        image: "images/cyberpunk_game_cover_1779123814120.png"
    },
    {
        id: 2,
        title: "Dragon's Legacy",
        category: "rpg",
        tags: ["RPG", "Fantasy", "Adventure"],
        price: "$49.99",
        image: "images/fantasy_rpg_cover_1779124275228.png"
    },
    {
        id: 3,
        title: "Stellar Odyssey",
        category: "sci-fi",
        tags: ["Sci-Fi", "Space", "Strategy"],
        price: "$39.99",
        image: "images/space_game_cover_1779124292142.png"
    }
];

const gameGrid = document.getElementById('gameGrid');
const searchInput = document.getElementById('searchInput');
const filterBtns = document.querySelectorAll('.filter-btn');

// Initialize
function init() {
    renderGames(games);
    setupEventListeners();
}

function renderGames(gamesToRender) {
    gameGrid.innerHTML = '';
    
    if (gamesToRender.length === 0) {
        gameGrid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: var(--text-muted);">No games found matching your criteria.</p>';
        return;
    }

    gamesToRender.forEach((game, index) => {
        const delay = index * 0.1; // Staggered animation
        
        const card = document.createElement('div');
        card.className = 'game-card';
        card.style.animationDelay = `${delay}s`;
        
        const tagsHtml = game.tags.map(tag => `<span class="tag">${tag}</span>`).join('');
        
        card.innerHTML = `
            <div class="card-image">
                <img src="${game.image}" alt="${game.title}">
            </div>
            <div class="card-content">
                <h3 class="game-title" title="${game.title}">${game.title}</h3>
                <div class="game-tags">
                    ${tagsHtml}
                </div>
                <div class="card-footer">
                    <span class="price">${game.price}</span>
                    <button class="buy-btn">Buy Now</button>
                </div>
            </div>
        `;
        
        gameGrid.appendChild(card);
    });
}

function setupEventListeners() {
    // Search
    searchInput.addEventListener('input', (e) => {
        const searchTerm = e.target.value.toLowerCase();
        const activeFilterBtn = document.querySelector('.filter-btn.active');
        const currentCategory = activeFilterBtn.dataset.filter;
        
        filterGames(searchTerm, currentCategory);
    });

    // Category Filter
    filterBtns.forEach(btn => {
        btn.addEventListener('click', (e) => {
            // Update active state
            filterBtns.forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');
            
            const currentCategory = e.target.dataset.filter;
            const searchTerm = searchInput.value.toLowerCase();
            
            filterGames(searchTerm, currentCategory);
        });
    });
}

function filterGames(searchTerm, category) {
    const filtered = games.filter(game => {
        const matchesSearch = game.title.toLowerCase().includes(searchTerm) || 
                              game.tags.some(tag => tag.toLowerCase().includes(searchTerm));
        const matchesCategory = category === 'all' || game.category === category;
        
        return matchesSearch && matchesCategory;
    });
    
    renderGames(filtered);
}

// Run init when DOM is loaded
document.addEventListener('DOMContentLoaded', init);
