const calm = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

/* The theme of the menu: only from a button. */
const theme = document.getElementById('theme') as HTMLAudioElement;
const toggles = document.querySelectorAll<HTMLButtonElement>('[data-theme-toggle]');
function showTheme(playing: boolean) {
  toggles.forEach(function (button) {
    button.setAttribute('aria-pressed', String(playing));
    button.textContent = playing ? 'Stop the theme' : 'Play the theme';
  });
}
toggles.forEach(function (button) {
  button.addEventListener('click', function () {
    if (theme.paused) {
      theme.volume = 0.6;
      theme.play().then(function () { showTheme(true); }, function () { showTheme(false); });
    } else {
      theme.pause();
      showTheme(false);
    }
  });
});

if (calm) {
  const loop = document.querySelector('.hero video') as HTMLVideoElement;
  loop.removeAttribute('autoplay');
  loop.pause();
} else if ('IntersectionObserver' in window) {
  document.querySelectorAll('.nights li').forEach(function (row) {
    row.querySelectorAll<HTMLElement>('.w i').forEach(function (light, i) { light.style.transitionDelay = 300 + i * 90 + 'ms'; });
  });
  const seen = new IntersectionObserver(function (entries) {
    entries.forEach(function (entry) {
      if (entry.isIntersecting) { entry.target.classList.add('in'); seen.unobserve(entry.target); }
    });
  }, { rootMargin: '0px 0px -8% 0px' });
  document.documentElement.classList.add('js-reveal');
  document.querySelectorAll('.reveal').forEach(function (el) { seen.observe(el); });
}
