import { Injectable } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import * as fs from 'fs';
import * as path from 'path';

@Injectable()
export class AuthService {
  private storagePath: string;

  constructor(private configService: ConfigService) {
    this.storagePath = this.configService.get('playwrightStoragePath') || './playwright/.auth/user.json';
  }

  async isAuthenticated(): Promise<boolean> {
    return fs.existsSync(this.storagePath);
  }

  async getStorageState(): Promise<string | undefined> {
    if (fs.existsSync(this.storagePath)) {
      return this.storagePath;
    }
    return undefined;
  }

  async saveStorageState(state: object): Promise<void> {
    const dir = path.dirname(this.storagePath);
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    fs.writeFileSync(this.storagePath, JSON.stringify(state));
  }

  async clearStorageState(): Promise<void> {
    if (fs.existsSync(this.storagePath)) {
      fs.unlinkSync(this.storagePath);
    }
  }
}
