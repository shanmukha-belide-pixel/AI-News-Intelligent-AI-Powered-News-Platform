// =============================================================================
// api_client.dart  —  Dio HTTP client with JWT interceptors
// =============================================================================
import 'package:dio/dio.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';

class ApiClient {
  static late final Dio dio;
  static late final Dio weatherDio;
  static const _baseUrl   = String.fromEnvironment(
      'API_BASE_URL', defaultValue: 'http://10.0.2.2:8080');

  static void initialize() {
    dio = Dio(BaseOptions(
      baseUrl: _baseUrl,
      connectTimeout: const Duration(seconds: 15),
      receiveTimeout: const Duration(seconds: 20),
      headers: {'Content-Type': 'application/json'},
    ));

    // JWT interceptor
    const storage = FlutterSecureStorage();
    dio.interceptors.add(InterceptorsWrapper(
      onRequest: (opts, handler) async {
        final token = await storage.read(key: 'access_token');
        if (token != null) opts.headers['Authorization'] = 'Bearer $token';
        handler.next(opts);
      },
      onError: (e, handler) async {
        if (e.response?.statusCode == 401) {
          // Try refresh
          final refreshToken = await storage.read(key: 'refresh_token');
          if (refreshToken != null) {
            try {
              final resp = await Dio().post('$_baseUrl/api/v1/auth/refresh',
                  data: {'refresh_token': refreshToken});
              final newToken = resp.data['data']['access_token'];
              await storage.write(key: 'access_token', value: newToken);
              e.requestOptions.headers['Authorization'] = 'Bearer $newToken';
              final retry = await dio.request(e.requestOptions.path,
                  options: Options(
                    method: e.requestOptions.method,
                    headers: e.requestOptions.headers,
                  ),
                  data: e.requestOptions.data);
              handler.resolve(retry);
              return;
            } catch (_) {}
          }
        }
        handler.next(e);
      },
    ));

    // Log interceptor (debug only)
    dio.interceptors.add(LogInterceptor(
      requestBody: false,
      responseBody: false,
      error: true,
    ));

    // Weather API client (direct to OWM)
    weatherDio = Dio(BaseOptions(
      baseUrl: 'https://api.openweathermap.org',
      connectTimeout: const Duration(seconds: 10),
      receiveTimeout: const Duration(seconds: 10),
    ));
  }
}
