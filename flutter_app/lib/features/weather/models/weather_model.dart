// =============================================================================
// weather_model.dart  —  Data model for current weather and AQI
// =============================================================================
import 'package:equatable/equatable.dart';

class WeatherModel extends Equatable {
  final String city;
  final String country;
  final double tempC;
  final double feelsLikeC;
  final double tempMaxC;
  final double tempMinC;
  final int humidityPct;
  final double windSpeedMs;
  final String condition;
  final String description;
  final String iconUrl;
  final int aqi;
  final String aqiLabel;

  const WeatherModel({
    required this.city,
    required this.country,
    required this.tempC,
    required this.feelsLikeC,
    required this.tempMaxC,
    required this.tempMinC,
    required this.humidityPct,
    required this.windSpeedMs,
    required this.condition,
    required this.description,
    required this.iconUrl,
    required this.aqi,
    required this.aqiLabel,
  });

  factory WeatherModel.fromJson(Map<String, dynamic> json) {
    return WeatherModel(
      city:          json['city'] ?? 'Unknown',
      country:       json['country'] ?? '',
      tempC:         (json['temp'] as num?)?.toDouble() ?? 0.0,
      feelsLikeC:    (json['feels_like'] as num?)?.toDouble() ?? 0.0,
      tempMaxC:      (json['temp_max'] as num?)?.toDouble() ?? 0.0,
      tempMinC:      (json['temp_min'] as num?)?.toDouble() ?? 0.0,
      humidityPct:   json['humidity'] ?? 0,
      windSpeedMs:   (json['wind_speed'] as num?)?.toDouble() ?? 0.0,
      condition:     json['condition'] ?? 'Clear',
      description:   json['description'] ?? '',
      iconUrl:       json['icon_url'] ?? '',
      aqi:           json['aqi'] ?? 0,
      aqiLabel:      json['aqi_label'] ?? '',
    );
  }

  Map<String, dynamic> toJson() => {
    'city':           city,
    'country':        country,
    'temp':           tempC,
    'feels_like':     feelsLikeC,
    'temp_max':       tempMaxC,
    'temp_min':       tempMinC,
    'humidity':       humidityPct,
    'wind_speed':     windSpeedMs,
    'condition':      condition,
    'description':    description,
    'icon_url':       iconUrl,
    'aqi':            aqi,
    'aqi_label':      aqiLabel,
  };

  @override
  List<Object?> get props => [
    city, country, tempC, feelsLikeC, tempMaxC, tempMinC,
    humidityPct, windSpeedMs, condition, description, iconUrl, aqi, aqiLabel,
  ];
}
