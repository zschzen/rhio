import { initTheme, setupThemeToggle } from './theme.js';
import { initExampleRuntime, loadExampleSource, setupRuntimeReload } from './runtime.js';
import { setupFilter } from './filter.js';
import home from './views/home.js';
import examplesList from './views/examples-list.js';
import exampleDetail from './views/example-detail.js';

const routes = [
  { rx: /^#\/?$/, render: () => home() },
  {
    rx: /^#\/examples\/?$/,
    render: () => examplesList(),
    after: () => setupFilter()
  },
  {
    rx: /^#\/example\/([a-zA-Z0-9_-]+)\/?$/,
    render: (m) => exampleDetail(m[1]),
    after: (m) => {
      setupRuntimeReload(m[1]);
      initExampleRuntime(m[1]);
      loadExampleSource(m[1]);
    }
  }
];

function updateNavIndicator(hash) {
  document.querySelectorAll('.header-nav a').forEach(link => link.classList.remove('active'));
  if (hash === '' || hash === '#/') document.getElementById('nav-about')?.classList.add('active');
  else if (hash.startsWith('#/example')) document.getElementById('nav-examples')?.classList.add('active');
}

function router() {
  const app = document.getElementById('app');
  const hash = window.location.hash || '#/';
  updateNavIndicator(hash);

  for (const route of routes) {
    const match = hash.match(route.rx);
    if (match) {
      app.innerHTML = route.render(match);
      window.scrollTo(0, 0);
      queueMicrotask(() => route.after?.(match));
      return;
    }
  }

  app.innerHTML = `<div class="container"><h2>404 Not Found</h2><p><a href="#/">Return home</a></p></div>`;
  window.scrollTo(0, 0);
}

window.addEventListener('hashchange', router);
window.addEventListener('DOMContentLoaded', () => {
  setupThemeToggle();
  initTheme();
  router();
});
