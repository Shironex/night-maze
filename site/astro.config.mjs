import { defineConfig, passthroughImageService } from 'astro/config';

export default defineConfig({
  site: 'https://shironex.github.io',
  base: '/night-maze',
  trailingSlash: 'always',
  // The pictures are already webp at their final size, so they are copied as they are.
  image: { service: passthroughImageService() },
});
