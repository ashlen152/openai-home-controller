import { Test, TestingModule } from '@nestjs/testing';
import { DoseEventsService } from '../../src/pump/services/dose-events.service';
import { getModelToken } from '@nestjs/mongoose';
import { DoseEvent } from '../../src/pump/schemas/dose-event.schema';
import { Model } from 'mongoose';
import { DoseEventStatus } from '../../src/pump/schemas/dose-event.schema';
import { CreateDoseEventDto } from '../../src/pump/dto/create-dose-event.dto';
import { MetadataDto } from '../../src/pump/dto/metadata.dto';

describe('DoseEventsService', () => {
  let service: DoseEventsService;
  let model: Model<DoseEvent>;

  const mockDoseEvent = {
    _id: '507f1f77bcf86cd799439011',
    pumpId: 'test_pump_001',
    eventId: '1234567890',
    timestamp: 1234567890,
    volume: 10.5,
    status: DoseEventStatus.COMPLETED,
    success: true,
    metadata: {
      totalToday: 25.5,
      remaining: 4.5,
      isAuto: true,
    },
    save: jest.fn().mockResolvedValue(true),
  };

  const mockModel = {
    find: jest.fn().mockReturnValue({ exec: jest.fn() }),
    create: jest.fn(),
  };

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [
        DoseEventsService,
        {
          provide: getModelToken('DoseEvent'),
          useValue: mockModel,
        },
      ],
    }).compile();

    service = module.get<DoseEventsService>(DoseEventsService);
    model = module.get<Model<DoseEvent>>(getModelToken('DoseEvent'));
  });

  afterEach(() => {
    jest.clearAllMocks();
  });

  describe('logDoseEvent', () => {
    it('should log started event with success null', async () => {
      const dto: CreateDoseEventDto = {
        pumpId: 'test_pump_001',
        eventId: '1234567890',
        timestamp: 1234567890,
        volume: 10.5,
        status: DoseEventStatus.STARTED,
        success: null,
        metadata: {
          totalToday: 10.5,
          remaining: 19.5,
          isAuto: true,
        },
      };

      mockModel.create.mockResolvedValue(mockDoseEvent as any);

      const result = await service.logDoseEvent(dto);
      expect(model.create).toHaveBeenCalledWith({
        ...dto,
      });
      expect(result).toEqual(mockDoseEvent);
    });

    it('should throw error for started event with non-null success', async () => {
      const dto: CreateDoseEventDto = {
        pumpId: 'test_pump_001',
        eventId: '1234567890',
        timestamp: 1234567890,
        volume: 10.5,
        status: DoseEventStatus.STARTED,
        success: true, // Should be null for started events
        metadata: {
          totalToday: 10.5,
          remaining: 19.5,
          isAuto: true,
        },
      };

      await expect(service.logDoseEvent(dto)).rejects.toThrow(
        'For started events, success must be null',
      );
    });

    it('should throw error for completed event with null success', async () => {
      const dto: CreateDoseEventDto = {
        pumpId: 'test_pump_001',
        eventId: '1234567890',
        timestamp: 1234567890,
        volume: 10.5,
        status: DoseEventStatus.COMPLETED,
        success: null, // Should be boolean for completed events
        metadata: {
          totalToday: 10.5,
          remaining: 19.5,
          isAuto: true,
        },
      };

      await expect(service.logDoseEvent(dto)).rejects.toThrow(
        'For completed/failed events, success must be boolean',
      );
    });

    it('should throw error for volume <= 0', async () => {
      const dto: CreateDoseEventDto = {
        pumpId: 'test_pump_001',
        eventId: '1234567890',
        timestamp: 1234567890,
        volume: 0, // Invalid volume
        status: DoseEventStatus.STARTED,
        success: null,
        metadata: {
          totalToday: 0,
          remaining: 30,
          isAuto: true,
        },
      };

      await expect(service.logDoseEvent(dto)).rejects.toThrow();
    });
  });

  describe('getDoseHistory', () => {
    it('should return dose history sorted by timestamp descending', async () => {
      const mockEvents = [mockDoseEvent, { ...mockDoseEvent, timestamp: 1234567800 } as any];
      mockModel.find().exec.mockResolvedValue(mockEvents);
      mockModel.find().sort.mockReturnValue({ exec: jest.fn().mockResolvedValue(mockEvents) });

      const result = await service.getDoseHistory('test_pump_001');
      expect(model.find).toHaveBeenCalledWith({ pumpId: 'test_pump_001' });
      expect(result).toEqual(mockEvents);
    });
  });

  describe('getTodaysDoses', () => {
    it('should return today doses filtered by date range', async () => {
      const mockEvents = [mockDoseEvent];
      mockModel.find().exec.mockResolvedValue(mockEvents);
      mockModel.find().sort.mockReturnValue({ exec: jest.fn().mockResolvedValue(mockEvents) });

      const result = await service.getTodaysDoses('test_pump_001');
      expect(model.find).toHaveBeenCalledWith({
        pumpId: 'test_pump_001',
        timestamp: {
          $gte: expect.any(Number),
          $lte: expect.any(Number),
        },
      });
      expect(result).toEqual(mockEvents);
    });
  });
});
