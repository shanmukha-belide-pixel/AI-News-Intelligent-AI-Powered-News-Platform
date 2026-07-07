// =============================================================================
// weather_provider.dart  —  Riverpod state management for current weather and AQI
// =============================================================================
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:geolocator/geolocator.dart';
import '../../../core/network/api_client.dart';
import '../models/weather_model.dart';

final weatherProvider =
    StateNotifierProvider<WeatherNotifier, AsyncValue<WeatherModel?>>((ref) {
  return WeatherNotifier();
});

class WeatherNotifier extends StateNotifier<AsyncValue<WeatherModel?>> {
  WeatherNotifier() : super(const AsyncValue.loading());

  Future<void> fetchCurrentLocation() async {
    try {
      state = const AsyncValue.loading();
      
      // Request location permissions
      LocationPermission permission = await Geolocator.checkPermission();
      if (permission == LocationPermission.denied) {
        permission = await Geolocator.requestPermission();
      }

      if (permission == LocationPermission.always || permission == LocationPermission.whileInUse) {
        Position pos = await Geolocator.getCurrentPosition(
            desiredAccuracy: LocationAccuracy.low);
        await fetchByCoords(pos.latitude, pos.longitude);
      } else {
        // Fallback to Mumbai if permission denied
        await fetchByCity('Mumbai');
      }
    } catch (e, st) {
      state = AsyncValue.error(e, st);
      // Mock weather fallback
      state = AsyncValue.data(_mockWeather('Mumbai', 28.5));
    }
  }

  Future<void> fetchByCity(String city) async {
    try {
      final response = await ApiClient.dio.get('/api/v1/weather/current',
          queryParameters: {'city': city});
      
      if (response.statusCode == 200 && response.data['success'] == true) {
        final data = WeatherModel.fromJson(response.data['data']);
        state = AsyncValue.data(data);
      } else {
        state = AsyncValue.data(_mockWeather(city, 28.5));
      }
    } catch (_) {
      state = AsyncValue.data(_mockWeather(city, 29.0));
    }
  }

  Future<void> fetchByCoords(double lat, double lon) async {
    try {
      final response = await ApiClient.dio.get('/api/v1/weather/current',
          queryParameters: {'lat': lat, 'lon': lon});
      
      if (response.statusCode == 200 && response.data['success'] == true) {
        final data = WeatherModel.fromJson(response.data['data']);
        state = AsyncValue.data(data);
      } else {
        state = AsyncValue.data(_mockWeather('Current Location', 26.0));
      }
    } catch (_) {
      state = AsyncValue.data(_mockWeather('Current Location', 26.0));
    }
  }

  WeatherModel _mockWeather(String city, double temp) {
    return WeatherModel(
      city: city,
      country: 'IN',
      tempC: temp,
      feelsLikeC: temp + 2.0,
      tempMaxC: temp + 4.0,
      tempMinC: temp - 3.0,
      humidityPct: 78,
      windSpeedMs: 4.5,
      condition: 'Clouds',
      description: 'scattered clouds',
      iconUrl: 'https://openweathermap.org/img/wn/03d@2x.png',
      aqi: 2,
      aqiLabel: 'Fair',
    );
  }
}
