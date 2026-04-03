import { Test, TestingModule } from '@nestjs/testing';
import { PumpSettingsService } from '../../src/pump/services/pump-settings.service';

describe('PumpSettingsService', () => {
  let service: PumpSettingsService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [PumpSettingsService],
    }).compile();

    service = module.get<PumpSettingsService>(PumpSettingsService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });

  it('should have getSettings method', () => {
    expect(typeof service.getSettings).toBe('function');
  });

  it('should have getAllPumps method', () => {
    expect(typeof service.getAllPumps).toBe('function');
  });

  it('should have upsertSettings method', () => {
    expect(typeof service.upsertSettings).toBe('function');
  });

  it('should have deleteSettings method', () => {
    expect(typeof service.deleteSettings).toBe('function');
  });
});
