/* Pump API client - typed fetch wrappers for pump-related endpoints */
type JSONValue = string | number | boolean | null | JSONValue[] | JSONObject;
interface JSONObject { [key: string]: JSONValue; }

// Public Pump type represents pump settings/model information returned by the API
export interface Pump {
  id: string;
  pumpId?: string;
  name?: string;
  model?: string;
  serial?: string;
  enabled?: boolean;
  createdAt?: string;
  updatedAt?: string;
}

// DTO for upsert/create pump settings
export type CreatePumpSettingsDto = JSONObject;

// Internal helper to parse JSON responses and surface meaningful errors
async function handleResponse<T>(resp: Response): Promise<T> {
  const contentType = resp.headers.get('content-type');
  const text = await resp.text();
  if (!resp.ok) {
    let message = resp.statusText;
    try {
      const data = contentType && contentType.includes('application/json')
        ? JSON.parse(text)
        : null;
      if (data && typeof data === 'object' && 'message' in data) {
        message = (data as any).message;
      } else if (data && typeof data === 'object' && 'error' in data) {
        message = (data as any).error;
      }
    } catch {
      // keep default message
    }
    throw new Error(message || `HTTP ${resp.status}`);
  }
  if (contentType && contentType.includes('application/json')) {
    try {
      return JSON.parse(text) as T;
    } catch {
      // In case of empty body or non-JSON, cast gracefully
      return ({} as unknown) as T;
    }
  }
  // Non-JSON response fallback
  return ({} as unknown) as T;
}

const BASE_API = '/api';

export async function getPumps(): Promise<Pump[]> {
  const resp = await fetch(`${BASE_API}/pump-settings`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<Pump[]>(resp);
}

export async function getPump(pumpId: string): Promise<Pump> {
  const resp = await fetch(`${BASE_API}/pump-settings/${encodeURIComponent(pumpId)}`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<Pump>(resp);
}

export async function upsertPump(data: CreatePumpSettingsDto): Promise<Pump> {
  const resp = await fetch(`${BASE_API}/pump-settings`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data),
  });
  return handleResponse<Pump>(resp);
}

export async function deletePump(pumpId: string): Promise<{ success: boolean }> {
  const resp = await fetch(`${BASE_API}/pump-settings/${encodeURIComponent(pumpId)}`, {
    method: 'DELETE',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ success: boolean }>(resp);
}

export async function testDose(pumpId: string, volume: number, speed?: number): Promise<{ commandId: string; status?: string }> {
  const payload = speed !== undefined ? { speed } : {};
  const resp = await fetch(`${BASE_API}/pump-commands/${encodeURIComponent(pumpId)}/test-dose/${encodeURIComponent(String(volume))}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  return handleResponse<{ commandId: string; status?: string }>(resp);
}

export interface PumpCommand { id: string; status?: string; }

export async function getPumpCommands(pumpId: string): Promise<{ pendingCommands: PumpCommand[] }> {
  const resp = await fetch(`${BASE_API}/pump-commands/${encodeURIComponent(pumpId)}`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ pendingCommands: PumpCommand[] }>(resp);
}

export async function completeCommand(pumpId: string, commandId: string, status: string): Promise<{ success: boolean }> {
  const resp = await fetch(`${BASE_API}/pump-commands/${encodeURIComponent(pumpId)}/complete`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ commandId, status }),
  });
  return handleResponse<{ success: boolean }>(resp);
}

export async function calibrateStart(pumpId: string): Promise<{ success: boolean; commandId: string }> {
  const resp = await fetch(`${BASE_API}/pump-commands/${encodeURIComponent(pumpId)}/calibrate/start`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ success: boolean; commandId: string }>(resp);
}

export async function calibrateSave(pumpId: string, measuredML: number): Promise<{ success: boolean }> {
  const resp = await fetch(`${BASE_API}/pump-commands/${encodeURIComponent(pumpId)}/calibrate/save`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ measuredML }),
  });
  return handleResponse<{ success: boolean }>(resp);
}

export async function getDoseEvents(pumpId: string): Promise<{ doses: JSONValue[] }> {
  const resp = await fetch(`${BASE_API}/dose-events/${encodeURIComponent(pumpId)}`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ doses: JSONValue[] }>(resp);
}

export async function getTodayDoseEvents(pumpId: string): Promise<{ totalToday: number }> {
  const resp = await fetch(`${BASE_API}/dose-events/${encodeURIComponent(pumpId)}/today`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ totalToday: number }>(resp);
}

export async function getSerialStatus(): Promise<{ connected: boolean }> {
  const resp = await fetch(`${BASE_API}/serial/status`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ connected: boolean }>(resp);
}

export async function getSerialLogs(): Promise<{ logs: string[]; connected: boolean }> {
  const resp = await fetch(`${BASE_API}/serial/logs`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ logs: string[]; connected: boolean }>(resp);
}

export async function getHealth(): Promise<{ status: string }> {
  const resp = await fetch(`${BASE_API}/health`, {
    method: 'GET',
    headers: { 'Content-Type': 'application/json' },
  });
  return handleResponse<{ status: string }>(resp);
}

// Re-exported type map for consumers
export type PumpApi = {
  getPumps: typeof getPumps;
  getPump: typeof getPump;
  upsertPump: typeof upsertPump;
  deletePump: typeof deletePump;
  testDose: typeof testDose;
  getPumpCommands: typeof getPumpCommands;
  completeCommand: typeof completeCommand;
  calibrateStart: typeof calibrateStart;
  calibrateSave: typeof calibrateSave;
  getDoseEvents: typeof getDoseEvents;
  getTodayDoseEvents: typeof getTodayDoseEvents;
  getSerialStatus: typeof getSerialStatus;
  getSerialLogs: typeof getSerialLogs;
  getHealth: typeof getHealth;
};

// Default export of types for easier import elsewhere if needed
