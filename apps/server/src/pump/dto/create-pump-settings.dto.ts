import {
  IsBoolean,
  IsNumber,
  IsString,
  IsOptional,
  Min,
  Max,
  Matches,
  registerDecorator,
  ValidationOptions,
  ValidationArguments,
} from 'class-validator';

export function IsNotEqualTo(property: string, validationOptions?: ValidationOptions) {
  return function (object: Object, propertyName: string) {
    registerDecorator({
      name: 'isNotEqualTo',
      target: object.constructor,
      propertyName: propertyName,
      constraints: [property],
      options: validationOptions,
      validator: {
        validate(value: any, args: ValidationArguments) {
          const [relatedPropertyName] = args.constraints;
          const relatedValue = (args.object as any)[relatedPropertyName];
          return value !== relatedValue;
        },
        defaultMessage(args: ValidationArguments) {
          const [relatedPropertyName] = args.constraints;
          return `${args.property} must not be equal to ${relatedPropertyName}`;
        },
      },
    });
  };
}

export class CreatePumpSettingsDto {
  @IsString()
  @Matches(/^[a-zA-Z0-9_]{1,15}$/, {
    message: 'pumpId must be alphanumeric or underscore and max 15 characters',
  })
  pumpId: string;

  @IsBoolean()
  enabled: boolean;

  @IsNumber()
  dailyVolume: number;

  @IsNumber()
  @Min(0)
  @Max(23)
  dayStartHour: number;

  @IsNumber()
  @Min(0)
  @Max(23)
  @IsNotEqualTo('dayStartHour', { message: 'dayStartHour and dayEndHour must not be equal' })
  dayEndHour: number;

  @IsNumber()
  @Min(0)
  @Max(100)
  dayPercent: number;

  @IsNumber()
  @Min(1)
  @Max(288)
  scheduleSlots: number;

  @IsNumber()
  stepsPerML: number;

  @IsNumber()
  @Min(0)
  activeProfile: number;

  @IsOptional()
  @IsNumber()
  @Min(0)
  pausedUntil?: number;
}
