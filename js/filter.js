let currentTagFilter = '';

function setTagFilter(tag) {
  currentTagFilter = tag;
  document.querySelectorAll('.filter-tile').forEach(btn => {
    btn.classList.toggle('active', btn.getAttribute('data-tag') === tag);
  });
  filterExamples();
}

export function filterExamples() {
  const queryInput = document.getElementById('search-input');
  const query = queryInput ? queryInput.value.toLowerCase() : '';
  const tag = currentTagFilter;

  let visible = 0;
  document.querySelectorAll('.example-item').forEach(el => {
    const title = (el.getAttribute('data-title') || '').toLowerCase();
    const tags = el.getAttribute('data-tags') || '';

    const matchesSearch = title.includes(query);
    const matchesTag = tag === '' || tags.split(',').includes(tag);
    const show = matchesSearch && matchesTag;

    el.style.display = show ? 'flex' : 'none';
    if (show) visible++;
  });

  const countEl = document.querySelector('[data-example-count]');
  if (countEl) countEl.textContent = String(visible);

  const empty = document.querySelector('[data-empty-state]');
  if (empty) empty.hidden = visible !== 0;
}

export function setupFilter() {
  currentTagFilter = '';

  const searchInput = document.getElementById('search-input');
  if (searchInput) {
    searchInput.addEventListener('input', filterExamples);
  }

  document.querySelectorAll('.filter-tile').forEach(btn => {
    btn.addEventListener('click', () => setTagFilter(btn.getAttribute('data-tag') || ''));
  });

  filterExamples();
}
