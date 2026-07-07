// =============================================================================
// main.dart  —  AI News Platform Flutter App Entry Point
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'package:firebase_core/firebase_core.dart';
import 'core/router/app_router.dart';
import 'core/theme/app_theme.dart';
import 'core/network/api_client.dart';
import 'services/notification_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Preferred orientations
  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);

  // Status bar style
  SystemChrome.setSystemUIOverlayStyle(const SystemUiOverlayStyle(
    statusBarColor: Colors.transparent,
    statusBarIconBrightness: Brightness.light,
  ));

  // Hive local storage
  await Hive.initFlutter();
  await Hive.openBox('settings');
  await Hive.openBox('cache');

  // Firebase (for push notifications)
  try {
    await Firebase.initializeApp();
    await NotificationService.initialize();
  } catch (_) {
    // Continue without Firebase in dev
  }

  // Initialize API client
  ApiClient.initialize();

  runApp(
    const ProviderScope(
      child: AINewsApp(),
    ),
  );
}

class AINewsApp extends ConsumerWidget {
  const AINewsApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final router    = ref.watch(appRouterProvider);
    final themeMode = ref.watch(themeModeProvider);

    return MaterialApp.router(
      title: 'AI News',
      debugShowCheckedModeBanner: false,
      theme:      AppTheme.light(),
      darkTheme:  AppTheme.dark(),
      themeMode:  themeMode,
      routerConfig: router,
      builder: (context, child) {
        // Global font scale limiter (accessibility)
        return MediaQuery(
          data: MediaQuery.of(context).copyWith(
            textScaler: TextScaler.linear(
              MediaQuery.of(context).textScaler.scale(1.0).clamp(0.8, 1.4),
            ),
          ),
          child: child!,
        );
      },
    );
  }
}
