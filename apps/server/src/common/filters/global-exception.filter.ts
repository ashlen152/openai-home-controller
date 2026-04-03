import {
  ExceptionFilter,
  Catch,
  ArgumentsHost,
  HttpException,
  HttpStatus,
  Logger,
} from '@nestjs/common';
import { Request, Response } from 'express';

@Catch()
export class GlobalExceptionFilter implements ExceptionFilter {
  private readonly logger = new Logger('HTTP');

  catch(exception: unknown, host: ArgumentsHost) {
    const ctx = host.switchToHttp();
    const response = ctx.getResponse<Response>();
    const request = ctx.getRequest<Request>();

    const status =
      exception instanceof HttpException
        ? exception.getStatus()
        : HttpStatus.INTERNAL_SERVER_ERROR;

    const message =
      exception instanceof HttpException
        ? exception.getResponse()
        : 'Internal server error';

    const errorDetail = exception instanceof Error ? exception.stack : String(exception);

    this.logger.error(
      `\n[FAILED] ${request.method} ${request.url}\n` +
      `  Status: ${status}\n` +
      `  Body: ${JSON.stringify(request.body)}\n` +
      `  Query: ${JSON.stringify(request.query)}\n` +
      `  Error: ${typeof message === 'string' ? message : JSON.stringify(message)}\n` +
      `  Stack: ${errorDetail}`,
    );

    response.status(status).json({
      statusCode: status,
      message: typeof message === 'string' ? message : (message as any).message,
      timestamp: new Date().toISOString(),
      path: request.url,
    });
  }
}
