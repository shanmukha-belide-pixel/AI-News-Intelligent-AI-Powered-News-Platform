// =============================================================================
// app_theme.dart  —  Material 3 Dark & Light themes with glassmorphism tokens
// =============================================================================
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

// ── Design Tokens ─────────────────────────────────────────────────────────────
class AppColors {
  // Brand
  static const primary     = Color(0xFF6C63FF);   // vibrant indigo
  static const primaryDark = Color(0xFF8F88FF);
  static const accent      = Color(0xFF00D4AA);   // teal accent
  static const accentWarm  = Color(0xFFFF6B6B);   // coral

  // Dark surface system
  static const darkBg      = Color(0xFF0A0E1A);   // deep navy black
  static const darkSurface = Color(0xFF111827);
  static const darkCard    = Color(0xFF1A2235);
  static const darkBorder  = Color(0xFF2A3347);
  static const darkText    = Color(0xFFEEF0F6);
  static const darkSubtext = Color(0xFF8A94A6);

  // Light surface system
  static const lightBg      = Color(0xFFF5F7FF);
  static const lightSurface = Color(0xFFFFFFFF);
  static const lightCard    = Color(0xFFFFFFFF);
  static const lightBorder  = Color(0xFFE2E8F0);
  static const lightText    = Color(0xFF1A202C);
  static const lightSubtext = Color(0xFF718096);

  // Category colors
  static const world        = Color(0xFF4A90D9);
  static const technology   = Color(0xFF7B68EE);
  static const business     = Color(0xFF2ECC71);
  static const politics     = Color(0xFFE74C3C);
  static const sports       = Color(0xFFF39C12);
  static const entertainment= Color(0xFFE91E63);
  static const health       = Color(0xFF00BCD4);
  static const science      = Color(0xFF9C27B0);
  static const climate      = Color(0xFF4CAF50);
  static const ai           = Color(0xFFFF5722);

  // Gradients
  static const List<Color> heroGradient  = [Color(0xFF6C63FF), Color(0xFF00D4AA)];
  static const List<Color> warmGradient  = [Color(0xFFFF6B6B), Color(0xFFFFBE76)];
  static const List<Color> darkGradient  = [Color(0xFF0A0E1A), Color(0xFF111827)];
  static const List<Color> newsGradient  = [Color(0xFF667EEA), Color(0xFF764BA2)];

  // Glassmorphism
  static const glassWhite    = Color(0x1AFFFFFF);
  static const glassDark     = Color(0x1A000000);
  static const glassBorder   = Color(0x33FFFFFF);
}

class AppTheme {
  // ── DARK THEME ─────────────────────────────────────────────────────────────
  static ThemeData dark() {
    final base = ThemeData.dark(useMaterial3: true);
    return base.copyWith(
      colorScheme: ColorScheme.fromSeed(
        seedColor: AppColors.primary,
        brightness: Brightness.dark,
        background: AppColors.darkBg,
        surface: AppColors.darkSurface,
        primary: AppColors.primary,
        secondary: AppColors.accent,
        onBackground: AppColors.darkText,
        onSurface: AppColors.darkText,
      ),
      scaffoldBackgroundColor: AppColors.darkBg,
      textTheme: _buildTextTheme(AppColors.darkText, AppColors.darkSubtext),
      appBarTheme: AppBarTheme(
        backgroundColor: AppColors.darkBg,
        foregroundColor: AppColors.darkText,
        elevation: 0,
        scrolledUnderElevation: 0,
        centerTitle: false,
        titleTextStyle: GoogleFonts.inter(
          color: AppColors.darkText,
          fontSize: 20,
          fontWeight: FontWeight.w700,
        ),
      ),
      cardTheme: CardThemeData(
        color: AppColors.darkCard,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(color: AppColors.darkBorder, width: 0.5),
        ),
        margin: EdgeInsets.zero,
      ),
      bottomNavigationBarTheme: const BottomNavigationBarThemeData(
        backgroundColor: AppColors.darkSurface,
        selectedItemColor: AppColors.primary,
        unselectedItemColor: AppColors.darkSubtext,
        type: BottomNavigationBarType.fixed,
        elevation: 0,
      ),
      chipTheme: ChipThemeData(
        backgroundColor: AppColors.darkCard,
        selectedColor: AppColors.primary.withOpacity(0.2),
        labelStyle: GoogleFonts.inter(fontSize: 12, color: AppColors.darkText),
        side: const BorderSide(color: AppColors.darkBorder),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: AppColors.darkCard,
        hintStyle: GoogleFonts.inter(color: AppColors.darkSubtext, fontSize: 14),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: AppColors.darkBorder),
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: AppColors.darkBorder),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: AppColors.primary, width: 1.5),
        ),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: AppColors.primary,
          foregroundColor: Colors.white,
          elevation: 0,
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 14),
          textStyle: GoogleFonts.inter(fontWeight: FontWeight.w600, fontSize: 15),
        ),
      ),
      dividerColor: AppColors.darkBorder,
      extensions: [_AppColorsExtension.dark],
    );
  }

  // ── LIGHT THEME ────────────────────────────────────────────────────────────
  static ThemeData light() {
    final base = ThemeData.light(useMaterial3: true);
    return base.copyWith(
      colorScheme: ColorScheme.fromSeed(
        seedColor: AppColors.primary,
        brightness: Brightness.light,
        background: AppColors.lightBg,
        surface: AppColors.lightSurface,
        primary: AppColors.primary,
        secondary: AppColors.accent,
        onBackground: AppColors.lightText,
        onSurface: AppColors.lightText,
      ),
      scaffoldBackgroundColor: AppColors.lightBg,
      textTheme: _buildTextTheme(AppColors.lightText, AppColors.lightSubtext),
      appBarTheme: AppBarTheme(
        backgroundColor: AppColors.lightBg,
        foregroundColor: AppColors.lightText,
        elevation: 0,
        scrolledUnderElevation: 0,
        centerTitle: false,
        titleTextStyle: GoogleFonts.inter(
          color: AppColors.lightText,
          fontSize: 20,
          fontWeight: FontWeight.w700,
        ),
      ),
      cardTheme: CardThemeData(
        color: AppColors.lightCard,
        elevation: 0,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(16),
          side: const BorderSide(color: AppColors.lightBorder, width: 0.5),
        ),
        margin: EdgeInsets.zero,
      ),
      extensions: [_AppColorsExtension.light],
    );
  }

  static TextTheme _buildTextTheme(Color primary, Color secondary) {
    return TextTheme(
      displayLarge:  GoogleFonts.inter(color: primary, fontWeight: FontWeight.w800, fontSize: 32),
      displayMedium: GoogleFonts.inter(color: primary, fontWeight: FontWeight.w700, fontSize: 28),
      displaySmall:  GoogleFonts.inter(color: primary, fontWeight: FontWeight.w700, fontSize: 24),
      headlineLarge: GoogleFonts.inter(color: primary, fontWeight: FontWeight.w700, fontSize: 22),
      headlineMedium:GoogleFonts.inter(color: primary, fontWeight: FontWeight.w600, fontSize: 20),
      headlineSmall: GoogleFonts.inter(color: primary, fontWeight: FontWeight.w600, fontSize: 18),
      titleLarge:    GoogleFonts.inter(color: primary, fontWeight: FontWeight.w600, fontSize: 16),
      titleMedium:   GoogleFonts.inter(color: primary, fontWeight: FontWeight.w500, fontSize: 15),
      titleSmall:    GoogleFonts.inter(color: primary, fontWeight: FontWeight.w500, fontSize: 14),
      bodyLarge:     GoogleFonts.merriweather(color: primary, fontSize: 16, height: 1.7),
      bodyMedium:    GoogleFonts.inter(color: primary, fontSize: 14, height: 1.6),
      bodySmall:     GoogleFonts.inter(color: secondary, fontSize: 12),
      labelLarge:    GoogleFonts.inter(color: primary, fontWeight: FontWeight.w600, fontSize: 14),
      labelMedium:   GoogleFonts.inter(color: secondary, fontSize: 12),
      labelSmall:    GoogleFonts.inter(color: secondary, fontSize: 11),
    );
  }
}

// ThemeMode provider (persisted in Hive)
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:hive_flutter/hive_flutter.dart';

final themeModeProvider = StateNotifierProvider<ThemeModeNotifier, ThemeMode>((ref) {
  return ThemeModeNotifier();
});

class ThemeModeNotifier extends StateNotifier<ThemeMode> {
  ThemeModeNotifier() : super(ThemeMode.dark) {
    final box = Hive.box('settings');
    final saved = box.get('theme_mode', defaultValue: 'dark') as String;
    state = saved == 'light' ? ThemeMode.light : ThemeMode.dark;
  }

  void toggle() {
    state = state == ThemeMode.dark ? ThemeMode.light : ThemeMode.dark;
    Hive.box('settings').put('theme_mode', state == ThemeMode.dark ? 'dark' : 'light');
  }
}

// Extension for accessing custom colors via Theme.of(context)
class _AppColorsExtension extends ThemeExtension<_AppColorsExtension> {
  static const dark  = _AppColorsExtension(isDark: true);
  static const light = _AppColorsExtension(isDark: false);
  final bool isDark;
  const _AppColorsExtension({required this.isDark});

  @override
  _AppColorsExtension copyWith({bool? isDark}) =>
      _AppColorsExtension(isDark: isDark ?? this.isDark);

  @override
  _AppColorsExtension lerp(_AppColorsExtension? other, double t) => this;
}
