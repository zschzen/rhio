import { db } from '../data.js';
import { escapeHtml } from '../util.js';

export default function examplesList() {
  const entries = Object.entries(db);
  const total = entries.length;
  const allTags = new Set();
  for (const ex of Object.values(db)) {
    if (ex.tags) ex.tags.forEach(t => allTags.add(t));
  }

  let tagButtons = `<button class="filter-tile active" data-tag="" type="button">All</button>`;
  Array.from(allTags).sort().forEach(t => {
    tagButtons += `<button class="filter-tile" data-tag="${escapeHtml(t)}" type="button">${escapeHtml(t)}</button>`;
  });

  let tiles = '';
  for (const [exId, ex] of entries) {
    const tagsAttr = ex.tags ? ex.tags.join(',') : '';
    const tagChips = ex.tags
      ? ex.tags.map(t => `<span class="tag">${escapeHtml(t)}</span>`).join('')
      : '';

    tiles += `
      <a href="#/example/${encodeURIComponent(exId)}" class="example-item thumb-card"
         data-title="${escapeHtml(ex.title)}" data-tags="${escapeHtml(tagsAttr)}">
        <div class="thumb-img" aria-hidden="true"></div>
        <div class="thumb-body">
          <div class="thumb-title">${escapeHtml(ex.title)}</div>
          <div class="thumb-tags">${tagChips}</div>
        </div>
        <svg class="tile-icon" focusable="false" preserveAspectRatio="xMidYMid meet" viewBox="0 0 32 32" aria-hidden="true">
          <path d="m18 6l-1.43 1.393L24.15 15H4v2h20.15l-7.58 7.573L18 26l10-10z"/>
        </svg>
      </a>`;
  }

  return `
    <div class="container">
      <p class="examples-intro">Run the WebAssembly builds generated from the rhio examples output.</p>

      <div class="filter-container">
        <div class="carbon-search">
          <svg focusable="false" preserveAspectRatio="xMidYMid meet" viewBox="0 0 32 32" aria-hidden="true">
            <path d="M29,27.58l-7.53-7.53C22.7,18.5,23.3,16.8,23.3,15c0-4.6-3.7-8.3-8.3-8.3S6.7,10.4,6.7,15s3.7,8.3,8.3,8.3c1.8,0,3.5-0.6,5.1-1.8l7.53,7.53L29,27.58z M8.7,15c0-3.5,2.8-6.3,6.3-6.3s6.3,2.8,6.3,6.3s-2.8,6.3-6.3,6.3S8.7,18.5,8.7,15z"></path>
          </svg>
          <input type="text" id="search-input" placeholder="Search examples" autocomplete="off">
        </div>
        <div class="filter-tags-list" role="group" aria-label="Filter by tag">
          ${tagButtons}
        </div>
      </div>

      <p class="gallery-meta">
        Showing <span data-example-count>${total}</span> of <span data-example-total>${total}</span>
      </p>

      <div data-layout="gallery">${tiles}</div>

      <div class="empty-state" data-empty-state hidden>
        <p>No examples match your filter.</p>
      </div>
    </div>
  `;
}
