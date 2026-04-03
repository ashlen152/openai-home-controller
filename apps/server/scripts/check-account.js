const CDP = require('chrome-remote-interface');

async function main() {
  const client = await CDP({ port: 9222 });
  const { Page, Runtime } = client;
  await Page.enable();
  await Runtime.enable();
  
  console.log('Navigating...');
  await Page.navigate({ url: 'https://www.tiktok.com/@cuxi037' });
  await Page.loadEventFired();
  await new Promise(r => setTimeout(r, 3000));
  
  // Scroll a few times
  for (let i = 0; i < 5; i++) {
    await Runtime.evaluate({
      expression: 'window.scrollBy(0, 2000)',
      awaitPromise: true
    });
    await new Promise(r => setTimeout(r, 1000));
  }
  
  // Check for links
  const result = await Runtime.evaluate({
    expression: `
      (() => {
        const links = document.querySelectorAll('a[href*="/video/"], a[href*="/photo/"]');
        const urls = [...new Set([...links].map(a => a.href))];
        const videos = urls.filter(u => u.includes('/video/') && !u.includes('/photo/'));
        const photos = urls.filter(u => u.includes('/photo/'));
        return { total: urls.length, videos: videos.length, photos: photos.length, sample: videos.slice(0,3) };
      })()
    `,
    returnByValue: true
  });
  
  console.log('Result:', JSON.stringify(result.result?.value, null, 2));
  await client.close();
}

main().catch(console.error);
