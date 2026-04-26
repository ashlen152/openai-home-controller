import * as os from 'os';
import { INestApplication } from '@nestjs/common';

export async function setupMdns(app: INestApplication, port: number = 3000) {
  if (!process.argv.includes('--mdns')) {
    return;
  }

  const { exec } = require('child_process');
  const ifaces = os.networkInterfaces();
  let targetIp = '127.0.0.1';

  for (const name of Object.keys(ifaces)) {
    for (const iface of ifaces[name] || []) {
      if (iface.family === 'IPv4' && !iface.internal && !name.includes('docker')) {
        targetIp = iface.address;
        if (name === 'en1') break;
      }
    }
    if (targetIp !== '127.0.0.1') break;
  }

  console.log(`Registering mDNS with IP: ${targetIp}`);
  exec(`dns-sd -P openai _http._tcp local ${port} openai.local. ${targetIp} &`);
  console.log(`mDNS advertised: http://openai.local:${port}`);
}