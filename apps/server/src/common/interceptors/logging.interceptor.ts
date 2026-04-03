import {
  Injectable,
  NestInterceptor,
  ExecutionContext,
  CallHandler,
  Logger,
} from '@nestjs/common';
import { Observable } from 'rxjs';
import { tap } from 'rxjs/operators';

@Injectable()
export class LoggingInterceptor implements NestInterceptor {
  private readonly logger = new Logger('HTTP');

  intercept(context: ExecutionContext, next: CallHandler): Observable<any> {
    const req = context.switchToHttp().getRequest();
    const method = req.method;
    const url = req.url;
    const body = req.body;
    const query = req.query;
    const now = Date.now();

    return next.handle().pipe(
      tap({
        next: (data) => {
          const res = context.switchToHttp().getResponse();
          this.logger.log(
            `[${method}] ${url} ${res.statusCode} — ${Date.now() - now}ms` +
            (body && Object.keys(body).length > 0 ? ` | Body: ${JSON.stringify(body)}` : '') +
            (query && Object.keys(query).length > 0 ? ` | Query: ${JSON.stringify(query)}` : ''),
          );
        },
        error: (err) => {
          const res = context.switchToHttp().getResponse();
          this.logger.error(
            `[${method}] ${url} ${res.statusCode || 500} FAILED — ${Date.now() - now}ms | ${err?.message || err}` +
            (body && Object.keys(body).length > 0 ? ` | Body: ${JSON.stringify(body)}` : '') +
            (query && Object.keys(query).length > 0 ? ` | Query: ${JSON.stringify(query)}` : ''),
          );
        },
      }),
    );
  }
}
