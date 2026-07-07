// =============================================================================
// app_router.dart  —  Central routing definitions (GoRouter)
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../features/home/home_screen.dart';

final appRouterProvider = Provider<GoRouter>((ref) {
  return GoRouter(
    initialLocation: '/',
    routes: [
      // ── Home Dashboard ─────────────────────────────────────────────────────
      GoRoute(
        path: '/',
        builder: (context, state) => const HomeScreen(),
      ),

      // ── Article Detail View ────────────────────────────────────────────────
      GoRoute(
        path: '/article/:id',
        builder: (context, state) {
          final id = state.pathParameters['id'] ?? '';
          return _DummyPage(title: 'Article Detail', subtitle: 'Article ID: $id');
        },
      ),

      // ── Search View ────────────────────────────────────────────────────────
      GoRoute(
        path: '/search',
        builder: (context, state) => const _DummyPage(title: 'Search News', subtitle: 'Full-text & Voice search'),
      ),

      // ── Profile / User settings ───────────────────────────────────────────
      GoRoute(
        path: '/profile',
        builder: (context, state) => const _DummyPage(title: 'User Profile', subtitle: 'Settings, Preferences, Streaks'),
      ),

      // ── Notifications ──────────────────────────────────────────────────────
      GoRoute(
        path: '/notifications',
        builder: (context, state) => const _DummyPage(title: 'Notifications', subtitle: 'Live alerts, Breaking news'),
      ),
    ],
    errorBuilder: (context, state) => const Scaffold(
      body: Center(child: Text('Page not found')),
    ),
  );
});

// ── Simple Page Placeholder for Development ──────────────────────────────────
class _DummyPage extends StatelessWidget {
  final String title;
  final String subtitle;
  const _DummyPage({required this.title, required this.subtitle});

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Scaffold(
      appBar: AppBar(
        title: Text(title),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => context.pop(),
        ),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.auto_awesome, size: 48, color: Colors.orange),
            const SizedBox(height: 12),
            Text(title, style: theme.textTheme.headlineMedium),
            const SizedBox(height: 6),
            Text(subtitle, style: theme.textTheme.bodyMedium?.copyWith(color: Colors.grey)),
          ],
        ),
      ),
    );
  }
}
