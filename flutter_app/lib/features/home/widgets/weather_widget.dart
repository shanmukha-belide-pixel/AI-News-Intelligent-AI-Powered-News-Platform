// =============================================================================
// weather_widget.dart  —  Home screen weather card with glassmorphism
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:cached_network_image/cached_network_image.dart';
import '../../../core/theme/app_theme.dart';
import '../../weather/models/weather_model.dart';
import '../../weather/providers/weather_provider.dart';
import 'dart:ui';

class WeatherWidget extends StatelessWidget {
  final AsyncValue<WeatherModel?> state;
  const WeatherWidget({super.key, required this.state});

  @override
  Widget build(BuildContext context) {
    return state.when(
      loading: () => _LoadingWeather(),
      error: (_, __) => const SizedBox.shrink(),
      data: (w) => w == null ? const SizedBox.shrink() : _WeatherCard(weather: w),
    );
  }
}

class _WeatherCard extends StatelessWidget {
  final WeatherModel weather;
  const _WeatherCard({required this.weather});

  @override
  Widget build(BuildContext context) {
    final condition = weather.condition.toLowerCase();
    final isRainy   = condition.contains('rain') || condition.contains('drizzle');
    final isClear   = condition.contains('clear');
    final isCloudy  = condition.contains('cloud');

    // Dynamic gradient based on condition + time
    final List<Color> gradient = isRainy
        ? [const Color(0xFF2C3E50), const Color(0xFF3498DB)]
        : isClear
            ? [const Color(0xFFFF8C00), const Color(0xFF6C63FF)]
            : [const Color(0xFF4A6FA5), const Color(0xFF6C63FF)];

    return ClipRRect(
      borderRadius: BorderRadius.circular(20),
      child: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            colors: gradient,
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: BackdropFilter(
          filter: ImageFilter.blur(sigmaX: 0, sigmaY: 0),
          child: Container(
            padding: const EdgeInsets.all(18),
            child: Row(
              children: [
                // Left: location + temp
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          const Icon(Icons.location_on, size: 14, color: Colors.white70),
                          const SizedBox(width: 4),
                          Text(weather.city,
                              style: const TextStyle(
                                  color: Colors.white70, fontSize: 13)),
                          Text(', ${weather.country}',
                              style: const TextStyle(
                                  color: Colors.white70, fontSize: 13)),
                        ],
                      ),
                      const SizedBox(height: 6),
                      Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text('${weather.tempC.round()}',
                              style: const TextStyle(
                                  color: Colors.white,
                                  fontSize: 48,
                                  fontWeight: FontWeight.w300,
                                  height: 1)),
                          const Text('°C',
                              style: TextStyle(
                                  color: Colors.white70, fontSize: 20)),
                        ],
                      ),
                      Text(weather.description,
                          style: const TextStyle(
                              color: Colors.white, fontSize: 14,
                              fontWeight: FontWeight.w500)),
                      const SizedBox(height: 6),
                      Text(
                          'Feels ${weather.feelsLikeC.round()}°  •  '
                          'H:${weather.tempMaxC.round()}°  L:${weather.tempMinC.round()}°',
                          style: const TextStyle(
                              color: Colors.white70, fontSize: 12)),
                    ],
                  ),
                ),

                // Right: icon + extra stats
                Column(
                  children: [
                    CachedNetworkImage(
                      imageUrl: weather.iconUrl,
                      width: 64, height: 64,
                      errorWidget: (_,__,___) => const Icon(
                          Icons.wb_sunny, size: 56, color: Colors.white70),
                    ).animate().scale(duration: 600.ms, curve: Curves.elasticOut),
                    const SizedBox(height: 8),
                    _Stat(Icons.water_drop_outlined,
                        '${weather.humidityPct}%'),
                    const SizedBox(height: 4),
                    _Stat(Icons.air, '${weather.windSpeedMs.round()} m/s'),
                    if (weather.aqiLabel.isNotEmpty) ...[
                      const SizedBox(height: 4),
                      _AqiChip(label: weather.aqiLabel, aqi: weather.aqi),
                    ],
                  ],
                ),
              ],
            ),
          ),
        ),
      ),
    ).animate().fadeIn(duration: 500.ms).slideY(begin: -0.15);
  }
}

class _Stat extends StatelessWidget {
  final IconData icon;
  final String value;
  const _Stat(this.icon, this.value);

  @override
  Widget build(BuildContext context) => Row(
    mainAxisSize: MainAxisSize.min,
    children: [
      Icon(icon, size: 12, color: Colors.white70),
      const SizedBox(width: 3),
      Text(value, style: const TextStyle(color: Colors.white70, fontSize: 12)),
    ],
  );
}

class _AqiChip extends StatelessWidget {
  final String label;
  final int aqi;
  const _AqiChip({required this.label, required this.aqi});

  @override
  Widget build(BuildContext context) {
    final color = [Colors.green, Colors.lightGreen, Colors.yellow,
                   Colors.orange, Colors.red][aqi.clamp(1,5) - 1];
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
      decoration: BoxDecoration(
        color: color.withOpacity(0.25),
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: color.withOpacity(0.5)),
      ),
      child: Text('AQI: $label',
          style: TextStyle(color: color, fontSize: 10,
              fontWeight: FontWeight.w600)),
    );
  }
}

class _LoadingWeather extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Container(
      height: 120,
      decoration: BoxDecoration(
        color: AppColors.darkCard,
        borderRadius: BorderRadius.circular(20),
      ),
    ).animate(onPlay: (c) => c.repeat())
     .shimmer(duration: 1200.ms, color: Colors.white.withOpacity(0.05));
  }
}
