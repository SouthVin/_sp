const games = [
    {
        id: 1,
        title: "Neon City: Overdrive",
        category: "cyberpunk",
        tags: ["Cyberpunk", "Action", "Open World"],
        price: 59.99,
        image: "images/cyberpunk_game_cover_1779123814120.png"
    },
    {
        id: 2,
        title: "Dragon's Legacy",
        category: "rpg",
        tags: ["RPG", "Fantasy", "Adventure"],
        price: 49.99,
        image: "images/fantasy_rpg_cover_1779124275228.png"
    },
    {
        id: 3,
        title: "Stellar Odyssey",
        category: "sci-fi",
        tags: ["Sci-Fi", "Space", "Strategy"],
        price: 39.99,
        image: "images/space_game_cover_1779124292142.png"
    }
];

const gameGrid = document.getElementById('gameGrid');
const searchInput = document.getElementById('searchInput');
const filterBtns = document.querySelectorAll('.filter-btn');

const cartToggle = document.getElementById('cartToggle');
const cartSidebar = document.getElementById('cartSidebar');
const cartOverlay = document.getElementById('cartOverlay');
const cartClose = document.getElementById('cartClose');
const cartItems = document.getElementById('cartItems');
const cartCount = document.getElementById('cartCount');
const cartTotal = document.getElementById('cartTotal');
const checkoutBtn = document.getElementById('checkoutBtn');
const cartNotification = document.getElementById('cartNotification');

const CART_STORAGE_KEY = 'nexus_games_cart';
let cart = [];
let notificationTimer;

function loadCart() {
    try {
        const stored = localStorage.getItem(CART_STORAGE_KEY);
        cart = stored ? JSON.parse(stored) : [];
    } catch {
        cart = [];
    }
}

function saveCart() {
    localStorage.setItem(CART_STORAGE_KEY, JSON.stringify(cart));
}

function getCartTotal() {
    return cart.reduce((sum, item) => sum + item.price * item.qty, 0);
}

function getCartItemCount() {
    return cart.reduce((sum, item) => sum + item.qty, 0);
}

function updateCartBadge() {
    const count = getCartItemCount();
    cartCount.textContent = count;
    cartCount.classList.add('bump');
    setTimeout(() => cartCount.classList.remove('bump'), 200);
}

function showNotification(message) {
    clearTimeout(notificationTimer);
    cartNotification.textContent = message;
    cartNotification.classList.add('show');
    notificationTimer = setTimeout(() => cartNotification.classList.remove('show'), 2000);
}

function openCart() {
    cartSidebar.classList.add('open');
    cartOverlay.classList.add('open');
    document.body.style.overflow = 'hidden';
}

function closeCart() {
    cartSidebar.classList.remove('open');
    cartOverlay.classList.remove('open');
    document.body.style.overflow = '';
}

function addToCart(game) {
    const existing = cart.find(item => item.id === game.id);
    if (existing) {
        existing.qty += 1;
    } else {
        cart.push({
            id: game.id,
            title: game.title,
            price: game.price,
            image: game.image,
            qty: 1
        });
    }
    saveCart();
    updateCartBadge();
    renderCart();
    showNotification(`"${game.title}" added to cart`);
}

function removeFromCart(id) {
    cart = cart.filter(item => item.id !== id);
    saveCart();
    updateCartBadge();
    renderCart();
}

function updateQty(id, delta) {
    const item = cart.find(item => item.id === id);
    if (!item) return;
    item.qty += delta;
    if (item.qty <= 0) {
        removeFromCart(id);
        return;
    }
    saveCart();
    updateCartBadge();
    renderCart();
}

function renderCart() {
    if (cart.length === 0) {
        cartItems.innerHTML = '<p class="cart-empty">Your cart is empty.</p>';
    } else {
        cartItems.innerHTML = cart.map(item => `
            <div class="cart-item">
                <div class="cart-item-image">
                    <img src="${item.image}" alt="${item.title}">
                </div>
                <div class="cart-item-details">
                    <h4 class="cart-item-title" title="${item.title}">${item.title}</h4>
                    <span class="cart-item-price">$${(item.price * item.qty).toFixed(2)}</span>
                    <div class="cart-item-actions">
                        <button class="qty-btn" data-action="decrease" data-id="${item.id}">-</button>
                        <span class="qty-value">${item.qty}</span>
                        <button class="qty-btn" data-action="increase" data-id="${item.id}">+</button>
                        <button class="cart-item-remove" data-action="remove" data-id="${item.id}" aria-label="Remove item">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                <polyline points="3 6 5 6 21 6"></polyline>
                                <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                            </svg>
                        </button>
                    </div>
                </div>
            </div>
        `).join('');
    }

    cartTotal.textContent = `$${getCartTotal().toFixed(2)}`;
    checkoutBtn.disabled = cart.length === 0;
}

function setupCartDelegation() {
    cartItems.addEventListener('click', (e) => {
        const btn = e.target.closest('button');
        if (!btn) return;

        const action = btn.dataset.action;
        const id = parseInt(btn.dataset.id, 10);

        if (action === 'increase') updateQty(id, 1);
        if (action === 'decrease') updateQty(id, -1);
        if (action === 'remove') removeFromCart(id);
    });
}

function init() {
    loadCart();
    renderGames(games);
    setupEventListeners();
    setupCartDelegation();
    updateCartBadge();
    renderCart();
}

function renderGames(gamesToRender) {
    gameGrid.innerHTML = '';

    if (gamesToRender.length === 0) {
        gameGrid.innerHTML = '<p style="grid-column: 1/-1; text-align: center; color: var(--text-muted);">No games found matching your criteria.</p>';
        return;
    }

    gamesToRender.forEach((game, index) => {
        const delay = index * 0.1;

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
                    <span class="price">$${game.price.toFixed(2)}</span>
                    <button class="buy-btn" data-add-cart="${game.id}">Add to Cart</button>
                </div>
            </div>
        `;

        gameGrid.appendChild(card);
    });
}

function setupEventListeners() {
    searchInput.addEventListener('input', (e) => {
        const searchTerm = e.target.value.toLowerCase();
        const activeFilterBtn = document.querySelector('.filter-btn.active');
        const currentCategory = activeFilterBtn.dataset.filter;

        filterGames(searchTerm, currentCategory);
    });

    filterBtns.forEach(btn => {
        btn.addEventListener('click', (e) => {
            filterBtns.forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');

            const currentCategory = e.target.dataset.filter;
            const searchTerm = searchInput.value.toLowerCase();

            filterGames(searchTerm, currentCategory);
        });
    });

    gameGrid.addEventListener('click', (e) => {
        const btn = e.target.closest('[data-add-cart]');
        if (!btn) return;
        const gameId = parseInt(btn.dataset.addCart, 10);
        const game = games.find(g => g.id === gameId);
        if (game) addToCart(game);
    });

    cartToggle.addEventListener('click', openCart);
    cartClose.addEventListener('click', closeCart);
    cartOverlay.addEventListener('click', closeCart);

    checkoutBtn.addEventListener('click', () => {
        if (cart.length === 0) return;
        showNotification(`Checkout successful! ${getCartItemCount()} items purchased.`);
        cart = [];
        saveCart();
        updateCartBadge();
        renderCart();
        closeCart();
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

document.addEventListener('DOMContentLoaded', init);
